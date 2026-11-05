#include "program.h"

#include "imgui.h"
#include "../utilities/imgui/addons/imgui_user.h"
#include "../utilities/imgui/std/imgui-stdlib.h"

#include "../structures/chart-metadata.h"
#include "../structures/configuration.h"

namespace
{
	Time WindowTimeBegin;
	Time WindowTimeEnd;

	Cursor EditCursor;

	TimefieldRenderGraph NoteRenderGraph;
	TimefieldRenderGraph PreviewRenderGraph;
	TimefieldRenderGraph WaveformRenderGraph;
	TimefieldRenderGraph DebugRenderGraph;

	Chart* SelectedChart = nullptr;

	float ZoomLevel = 1.0f;
	int CurrentSnap = 2;

	ChartMetadata ChartMetadataSetup;

	Configuration Config;

	bool ShouldSetUpMetadata = false;
	bool ShouldSetUpNewChart = false;

	std::string TimeToGo = "0";

	int ZoomIndex = 5;
	std::vector<float> LegalZoomLevels = { 0.25f, 0.4f, 0.5f, 0.6f, 0.75f, 1.0f, 1.25f, 1.4f, 1.5f, 2.0f, 2.5f, 3.0f, 3.5f, 4.0f };
}

//module includes
#include "../modules/imgui-module.h"
#include "../modules/chart-parser-module.h"
#include "../modules/dialog-module.h"
#include "../modules/timefield-render-module.h"
#include "../modules/audio-module.h"
#include "../modules/input-module.h"
#include "../modules/beat-module.h"
#include "../modules/edit-module.h"
#include "../modules/background-module.h"
#include "../modules/minimap-module.h"
#include "../modules/waveform-module.h"
#include "../modules/popup-module.h"
#include "../modules/notification-module.h"
#include "../modules/shortcut-menu-module.h"
#include "../modules/debug-module.h"

void Program::RegisterModules()
{
	ModuleManager::Register<BackgroundModule>();
	ModuleManager::Register<TimefieldRenderModule>();
	ModuleManager::Register<ImGuiModule>();
	ModuleManager::Register<DialogModule>();
	ModuleManager::Register<PopupModule>();
	ModuleManager::Register<InputModule>();
	ModuleManager::Register<ShortcutMenuModule>();
	ModuleManager::Register<MiniMapModule>();
	ModuleManager::Register<NotificationModule>();
	ModuleManager::Register<ChartParserModule>();
	ModuleManager::Register<AudioModule>();
	ModuleManager::Register<WaveFormModule>();
	ModuleManager::Register<BeatModule>();
	ModuleManager::Register<EditModule>();
	ModuleManager::Register<DebugModule>();
}

void Program::InnerStartUp()
{
	MOD(ImGuiModule).Init(_RenderWindow);
	MOD(NotificationModule).SetStartY(_WindowMetrics.MenuBarHeight + 16);

	if(Config.Load())
	{
		SetConfig(Config);
		PUSH_NOTIFICATION("Config loaded");
	}
	else PUSH_NOTIFICATION("Config file not found. Created a new one");
}

void Program::InnerTick()
{
	MenuBar();

	if (_ShowDashboard)
		DisplayDashboard();

	if(ShouldSetUpMetadata)
		SetUpMetadata();

	if (!SelectedChart)
		return;

	UpdateCursor();
	MOD(EditModule).SetCursorData(EditCursor);

	WindowTimeBegin = MOD(TimefieldRenderModule).GetWindowTimePointBegin(MOD(AudioModule).GetTimeMilliSeconds(), ZoomLevel);
	WindowTimeEnd = MOD(TimefieldRenderModule).GetWindowTimePointEnd(MOD(AudioModule).GetTimeMilliSeconds(), ZoomLevel);

	if (!ImGui::GetIO().WantCaptureMouse && !ImGui::GetIO().WantTextInput)
		InputActions();

	MOD(TimefieldRenderModule).UpdateMetrics(_WindowMetrics);
	MOD(BeatModule).GenerateTimeRangeBeatLines(WindowTimeBegin, WindowTimeEnd, SelectedChart, CurrentSnap);

    // Metronome Logic
    if (_MetronomeEnabled)
    {
        Time currentTime = MOD(AudioModule).GetTimeMilliSeconds();
        if (_LastTickTime != 0 && currentTime > _LastTickTime)
        {
            // Check for beats between _LastTickTime and currentTime
            // We can ask BeatModule for the next beat line after LastTickTime
            BeatLine nextBeat = MOD(BeatModule).GetNextBeatLine(_LastTickTime);
            if (nextBeat.TimePoint <= currentTime && nextBeat.BeatSnap == 1) // Only major beats (4th)?
            {
                // Play tick
                MOD(AudioModule).PlayMetronomeTick();
            }
        }
        _LastTickTime = currentTime;
    }

    // Assist Tick Logic
    if (_AssistTickEnabled)
    {
        Time currentTime = MOD(AudioModule).GetTimeMilliSeconds();
        if (_LastAssistTickTime != 0 && currentTime > _LastAssistTickTime)
        {
             bool play = false;
             // Check notes slightly broader to catch frame misses? No, IterateNotesInTimeRange is inclusive?
             // Need to check implementation.
             SelectedChart->IterateNotesInTimeRange(_LastAssistTickTime, currentTime, [&](Note& n, Column c){
                 if (n.TimePoint > _LastAssistTickTime && n.TimePoint <= currentTime)
                     play = true;
             });

             if (play) MOD(AudioModule).PlayHitsound();
        }
        _LastAssistTickTime = currentTime;
    }

	SelectedChart->IterateNotesInTimeRange(WindowTimeBegin - TIMESLICE_LENGTH, WindowTimeEnd, [this](Note &InNote, const Column InColumn) {
		sf::Int8 alpha = 255;

		if (MOD(EditModule).IsEditModeActive<BpmEditMode>())
			alpha = 128;

		NoteRenderGraph.SubmitNoteRenderCommand(InNote, InColumn, alpha);
	});

	MOD(EditModule).SubmitToRenderGraph(PreviewRenderGraph, WindowTimeBegin, WindowTimeEnd);
}

void Program::InnerRender(sf::RenderTarget *const InOutRenderTarget)
{
	if (!SelectedChart)
		return;

    MOD(BackgroundModule).RenderBack(InOutRenderTarget);
    MOD(TimefieldRenderModule).RenderBack(InOutRenderTarget);

	if(Config.ShowWaveform)
		MOD(WaveFormModule).RenderWaveForm(WaveformRenderGraph, WindowTimeBegin, WindowTimeEnd, MOD(TimefieldRenderModule).GetTimefieldMetrics().LeftSidePosition + MOD(TimefieldRenderModule).GetTimefieldMetrics().FieldWidthHalf, ZoomLevel, InOutRenderTarget->getView().getSize().y);
		//MOD(WaveFormModule).RenderWaveFormPolygon(InOutRenderTarget, WindowTimeBegin, WindowTimeEnd, MOD(TimefieldRenderModule).GetTimefieldMetrics().LeftSidePosition + MOD(TimefieldRenderModule).GetTimefieldMetrics().FieldWidthHalf, ZoomLevel, InOutRenderTarget->getView().getSize().y);

	MOD(BeatModule).IterateThroughBeatlines([this, &InOutRenderTarget](const BeatLine &InBeatLine)
	{
		MOD(TimefieldRenderModule).RenderBeatLine(InOutRenderTarget, InBeatLine.TimePoint, InBeatLine.BeatSnap, MOD(AudioModule).GetTimeMilliSeconds(), ZoomLevel, InBeatLine.IsMeasure);
	});

    SelectedChart->IterateAllBpmPoints([this, &InOutRenderTarget](BpmPoint& bpm){
        if (bpm.TimePoint >= WindowTimeBegin && bpm.TimePoint <= WindowTimeEnd)
            MOD(TimefieldRenderModule).RenderTimingEvent(InOutRenderTarget, bpm.TimePoint, MOD(AudioModule).GetTimeMilliSeconds(), ZoomLevel, sf::Color(0, 255, 255, 128));
    });

    SelectedChart->IterateAllStops([this, &InOutRenderTarget](StopPoint& stop){
        if (stop.TimePoint >= WindowTimeBegin && stop.TimePoint <= WindowTimeEnd)
            MOD(TimefieldRenderModule).RenderTimingEvent(InOutRenderTarget, stop.TimePoint, MOD(AudioModule).GetTimeMilliSeconds(), ZoomLevel, sf::Color(255, 255, 0, 128));
    });

	MOD(DebugModule).RenderTimeSliceBoundaries(DebugRenderGraph, SelectedChart, WindowTimeBegin, WindowTimeEnd);

	MOD(TimefieldRenderModule).RenderTimefieldGraph(InOutRenderTarget, WaveformRenderGraph, MOD(AudioModule).GetTimeMilliSeconds(), ZoomLevel);
	MOD(TimefieldRenderModule).RenderTimefieldGraph(InOutRenderTarget, DebugRenderGraph, MOD(AudioModule).GetTimeMilliSeconds(), ZoomLevel, false);

	MOD(TimefieldRenderModule).RenderReceptors(InOutRenderTarget, CurrentSnap);

	MOD(TimefieldRenderModule).RenderTimefieldGraph(InOutRenderTarget, NoteRenderGraph, MOD(AudioModule).GetTimeMilliSeconds(), ZoomLevel);
	MOD(TimefieldRenderModule).RenderTimefieldGraph(InOutRenderTarget, PreviewRenderGraph, MOD(AudioModule).GetTimeMilliSeconds(), ZoomLevel, false);

	MOD(MiniMapModule).Render(InOutRenderTarget);

	if (MOD(MiniMapModule).ShouldPreview()) // :D ???
		MOD(MiniMapModule).RenderPreview(InOutRenderTarget, MOD(TimefieldRenderModule).GetRenderedTimefieldGraphSegment(MOD(MiniMapModule).GetPreviewRenderGraph(SelectedChart), MOD(MiniMapModule).GetHoveredTime(), ZoomLevel));

	NoteRenderGraph.ClearRenderCommands();
	PreviewRenderGraph.ClearRenderCommands();
	WaveformRenderGraph.ClearRenderCommands();
	DebugRenderGraph.ClearRenderCommands();
}

void Program::InnerShutDown()
{
	delete SelectedChart;
}

//************************************************************************************************************************************************************************************

namespace
{
    void HelpMarker(const char* desc)
    {
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
}

void Program::DisplayDashboard()
{
    ImGui::SetNextWindowSize(ImVec2(480, 620), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Dashboard & Control Center", &_ShowDashboard, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return;
    }

    if (ImGui::CollapsingHeader("1. Playback & Audio Controls", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::Button("Toggle Play/Pause (Space)", ImVec2(200, 0)))
        {
            MOD(AudioModule).TogglePause();
        }
        ImGui::SameLine();
        if (ImGui::Button("Play Selection (Shift+Space)", ImVec2(200, 0)) && SelectedChart)
        {
            Time start, end;
            if (MOD(EditModule).GetSelectionRange(start, end))
                MOD(AudioModule).PlayRange(start, end);
            else
                PUSH_NOTIFICATION("No notes selected");
        }
        ImGui::SameLine();
        HelpMarker("Toggle song playback or play strictly within the selected notes timestamp range.");

        int curTime = MOD(AudioModule).GetTimeMilliSeconds();
        ImGui::Text("Current Time: %d ms (%.2fs)", curTime, curTime / 1000.0f);

        float currentSpeed = MOD(AudioModule).GetPlaybackSpeed();
        ImGui::Text("Playback Speed: %.2fx", currentSpeed);
        ImGui::SameLine();
        if (ImGui::Button("-0.05x"))
        {
            MOD(AudioModule).ChangeSpeed(-0.05f);
        }
        ImGui::SameLine();
        if (ImGui::Button("+0.05x"))
        {
            MOD(AudioModule).ChangeSpeed(0.05f);
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset 1.0x"))
        {
            MOD(AudioModule).ResetSpeed();
        }
        ImGui::SameLine();
        HelpMarker("Adjust playback rate with or without pitch compensation based on audio pitch option.");

        if (ImGui::Checkbox("Metronome Tick", &_MetronomeEnabled))
        {
            PUSH_NOTIFICATION(_MetronomeEnabled ? "Metronome ON" : "Metronome OFF");
        }
        ImGui::SameLine();
        HelpMarker("Produces synthesized audio metronome beat clicks on major beats.");

        ImGui::SameLine();
        if (ImGui::Checkbox("Assist Tick (Hitsound)", &_AssistTickEnabled))
        {
            PUSH_NOTIFICATION(_AssistTickEnabled ? "Assist Tick ON" : "Assist Tick OFF");
        }
        ImGui::SameLine();
        HelpMarker("Procedurally synthesizes audio hitsounds when passing notes during playback.");

        if (ImGui::Button("Snap to Waveform Peak (Ctrl+P)") && SelectedChart)
        {
            SnapToPeak();
        }
        ImGui::SameLine();
        HelpMarker("Aligns current playback cursor to the closest audio waveform amplitude peak within 50ms.");

        ImGui::SameLine();
        if (ImGui::Button("Go To Time..."))
        {
            GoToTimePoint();
        }
    }

    if (ImGui::CollapsingHeader("2. Note Editing & Transformations", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::Button("Undo (Ctrl+Z)") && SelectedChart && MOD(EditModule).OnUndo())
            PUSH_NOTIFICATION("Undo");
        ImGui::SameLine();
        if (ImGui::Button("Redo (Ctrl+Y)") && SelectedChart && MOD(EditModule).OnRedo())
            PUSH_NOTIFICATION("Redo");
        ImGui::SameLine();
        HelpMarker("Step backwards and forwards through note modifications history.");

        ImGui::Separator();

        if (ImGui::Button("Quantize Selected (Ctrl+Q)") && SelectedChart)
        {
            MOD(EditModule).OnQuantize(CurrentSnap);
            PUSH_NOTIFICATION("Quantized selection to snap 1/%d", CurrentSnap);
        }
        ImGui::SameLine();
        HelpMarker("Snaps all selected notes to the active beat division based on active BPM points.");

        if (ImGui::Button("Mirror (Ctrl+H)") && SelectedChart)
        {
            MOD(EditModule).OnMirror();
            PUSH_NOTIFICATION("Mirrored selection columns");
        }
        ImGui::SameLine();
        if (ImGui::Button("Reverse (Ctrl+R)") && SelectedChart)
        {
            MOD(EditModule).OnReverse();
            PUSH_NOTIFICATION("Reversed notes in selection");
        }
        ImGui::SameLine();
        if (ImGui::Button("Shuffle (Ctrl+J)") && SelectedChart)
        {
            MOD(EditModule).OnShuffle();
            PUSH_NOTIFICATION("Shuffled note columns");
        }

        if (ImGui::Button("Expand 2.0x (Ctrl+Up)") && SelectedChart)
        {
            MOD(EditModule).OnExpand();
            PUSH_NOTIFICATION("Expanded selection spacing 2x");
        }
        ImGui::SameLine();
        if (ImGui::Button("Compress 0.5x (Ctrl+Down)") && SelectedChart)
        {
            MOD(EditModule).OnCompress();
            PUSH_NOTIFICATION("Compressed selection spacing 0.5x");
        }
        ImGui::SameLine();
        HelpMarker("Scales selected notes in time relative to the selection origin.");

        if (ImGui::Button("Convert Selection to Holds") && SelectedChart)
        {
            Time currentTime = MOD(AudioModule).GetTimeMilliSeconds();
            BpmPoint* bpm = MOD(BeatModule).GetClosestBeatLineToTimePoint(currentTime).TimePoint > 0 ? SelectedChart->GetPreviousBpmPointFromTimePoint(currentTime) : nullptr;
            if (!bpm) bpm = SelectedChart->GetNextBpmPointFromTimePoint(-1000000);
            double length = (bpm && CurrentSnap > 0) ? (bpm->BeatLength * (4.0 / CurrentSnap)) : 500.0;
            if (length <= 0) length = 500.0;
            MOD(EditModule).OnConvertToHolds(Time(length));
            PUSH_NOTIFICATION("Converted selection to holds");
        }
        ImGui::SameLine();
        if (ImGui::Button("Convert Selection to Taps") && SelectedChart)
        {
            MOD(EditModule).OnConvertToTaps();
            PUSH_NOTIFICATION("Converted holds to taps");
        }
        ImGui::SameLine();
        HelpMarker("Transforms selected tap notes into hold notes with snap duration, or hold notes into taps.");
    }

    if (ImGui::CollapsingHeader("3. Timing & Synchronization Tools", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (SelectedChart)
        {
            Time cur = MOD(AudioModule).GetTimeMilliSeconds();
            BpmPoint* curBpm = SelectedChart->GetPreviousBpmPointFromTimePoint(cur);
            if (curBpm)
                ImGui::Text("Active BPM: %.2f (BeatLength: %.1fms)", curBpm->Bpm, curBpm->BeatLength);
            else
                ImGui::Text("Active BPM: None detected before cursor");
        }

        if (ImGui::Button("Estimate BPM (Ctrl+B)") && SelectedChart)
        {
            MOD(EditModule).OnEstimateBPM();
            PUSH_NOTIFICATION("Estimating BPM via audio analysis...");
        }
        ImGui::SameLine();
        HelpMarker("Automatically calculates audio stream tempo using BASS_FX autocorrelation.");

        ImGui::SameLine();
        if (ImGui::Button("Tap BPM (Shift+T)") && SelectedChart)
        {
            MOD(EditModule).OnTap();
        }
        ImGui::SameLine();
        HelpMarker("Records tapping cadence in real time to calculate and inject average song BPM.");

        if (ImGui::Button("Move Entire Chart (All Notes / Timings)...") && SelectedChart)
        {
            OpenMoveAllNotes();
        }
        ImGui::SameLine();
        HelpMarker("Synchronously shifts all Notes, Holds, BPMs, Stops, SVs, and Time Signatures by a millisecond offset.");
    }

    if (ImGui::CollapsingHeader("4. Stream Pattern Generator"))
    {
        static int genStart = 0;
        static int genEnd = 0;
        static int genDivisor = 16;
        static int genPattern = 0;

        if (genStart == 0 && genEnd == 0 && SelectedChart)
        {
            genStart = WindowTimeBegin;
            genEnd = WindowTimeEnd;
        }

        ImGui::InputInt("Start (ms)", &genStart);
        ImGui::InputInt("End (ms)", &genEnd);
        const char* patternNames[] = { "Staircase", "Trill", "Spiral", "Random", "Jumpstream", "Handstream", "Chordjack" };
        ImGui::Combo("Pattern Style", &genPattern, patternNames, IM_ARRAYSIZE(patternNames));
        ImGui::InputInt("Divisor (Snap 4, 8, 16...)", &genDivisor);
        if (genDivisor < 1) genDivisor = 1;

        if (ImGui::Button("Generate Stream Pattern") && SelectedChart)
        {
            SelectedChart->GenerateStream(genStart, genEnd, genDivisor, (StreamPattern)genPattern);
            MOD(BeatModule).RecalculateSnaps(SelectedChart, genStart, genEnd);
            PUSH_NOTIFICATION("Generated %s stream pattern", patternNames[genPattern]);
        }
        ImGui::SameLine();
        HelpMarker("Synthesizes procedural rhythm stream patterns with automatic snap recalculation.");
    }

    if (ImGui::CollapsingHeader("5. NPS & Difficulty Analytics"))
    {
        if (SelectedChart)
        {
            static int npsWindow = 1000;
            ImGui::SliderInt("Window (ms)", &npsWindow, 250, 5000);
            std::vector<float> graph = SelectedChart->CalculateNPSGraph(npsWindow);
            if (!graph.empty())
            {
                ImGui::PlotLines("Notes Per Sec", graph.data(), graph.size(), 0, nullptr, 0.0f, FLT_MAX, ImVec2(0, 90));
            }
            ImGui::Text("Average NPS: %.2f | Peak NPS: %.2f", SelectedChart->GetAverageNPS(), SelectedChart->GetPeakNPS());
        }
        else
        {
            ImGui::TextDisabled("Load a chart to inspect difficulty density metrics.");
        }
    }

    if (ImGui::CollapsingHeader("6. View & Grid Settings"))
    {
        if (ImGui::Checkbox("Show Column Dividers", &Config.ShowColumnLines))
        {
            MOD(TimefieldRenderModule).GetSkin().ShowColumnLines = Config.ShowColumnLines;
            Config.Save();
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Show Audio Waveform", &Config.ShowWaveform))
        {
            Config.Save();
        }

        if (ImGui::Checkbox("Column Heatmap Density", &Config.ShowColumnHeatmap))
        {
            EditMode::static_Flags.ShowColumnHeatmap = Config.ShowColumnHeatmap;
            Config.Save();
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Auto-Timing Assistance", &Config.UseAutoTiming))
        {
            EditMode::static_Flags.UseAutoTiming = Config.UseAutoTiming;
            Config.Save();
        }

        int snapIdx = CurrentSnap;
        if (ImGui::InputInt("Beat Snap Divisor", &CurrentSnap))
        {
            if (CurrentSnap < 1) CurrentSnap = 1;
        }
        ImGui::SameLine();
        HelpMarker("Beat snap divisor used for note placement, quantization, and hold lengths (e.g. 4=1/4, 8=1/8, 16=1/16).");
    }

    if (ImGui::CollapsingHeader("7. Session & File Management"))
    {
        if (ImGui::Button("Quick Save (Ctrl+S)") && SelectedChart)
        {
            MOD(ChartParserModule).ExportChartSet(SelectedChart);
            PUSH_NOTIFICATION("Saved chart");
        }
        ImGui::SameLine();
        if (ImGui::Button("Edit Metadata (Ctrl+E)") && SelectedChart)
        {
            ShouldSetUpMetadata = true;
        }

        static bool hasValidated = false;
        static ChartValidationResult validationResult;

        if (ImGui::Button("Run Chart Integrity Check") && SelectedChart)
        {
            validationResult = SelectedChart->ValidateChart();
            hasValidated = true;
            if (validationResult.IsValid && validationResult.WarningCount == 0)
                PUSH_NOTIFICATION("Integrity check passed cleanly");
            else
                PUSH_NOTIFICATION("Integrity check: %d errors, %d warnings", validationResult.ErrorCount, validationResult.WarningCount);
        }
        ImGui::SameLine();
        HelpMarker("Scans entire chart for overlapping notes, out-of-range columns, and malformed hold lengths.");

        if (hasValidated)
        {
            if (validationResult.IsValid && validationResult.WarningCount == 0)
            {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Status: Chart is valid with 0 errors.");
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Status: %d Errors, %d Warnings", validationResult.ErrorCount, validationResult.WarningCount);
                for (size_t i = 0; i < std::min((size_t)5, validationResult.Issues.size()); ++i)
                {
                    ImGui::BulletText("%s", validationResult.Issues[i].c_str());
                }
                if (validationResult.Issues.size() > 5)
                {
                    ImGui::TextDisabled("...and %zu more issues", validationResult.Issues.size() - 5);
                }
            }
        }

        ImGui::Separator();
        if (ImGui::Button("Exit Application"))
        {
            _ShouldExitProgram = true;
        }
        ImGui::SameLine();
        HelpMarker("Safely flushes and terminates the editor session and window.");
    }

    ImGui::End();
}

void Program::MenuBar()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (MOD(ShortcutMenuModule).BeginMenu("File"))
		{
			if (MOD(ShortcutMenuModule).MenuItem("New Chart", sf::Keyboard::Key::LControl, sf::Keyboard::Key::N))
			{
				ChartMetadataSetup = ChartMetadata();
				ShouldSetUpNewChart = true;
				ShouldSetUpMetadata = true;
			}

			if (MOD(ShortcutMenuModule).MenuItem("Open", sf::Keyboard::Key::LControl, sf::Keyboard::Key::O))
			{
				MOD(DialogModule).OpenFileDialog(".osu;.sm", [this](const std::string &InPath)
				{
					OpenChart(InPath);
				});
			}

			MOD(ShortcutMenuModule).Separator();

			if (MOD(ShortcutMenuModule).MenuItem("Edit Metadata", sf::Keyboard::Key::LControl, sf::Keyboard::Key::E) && SelectedChart)
				ShouldSetUpMetadata = true;

			if (MOD(ShortcutMenuModule).MenuItem("Save", sf::Keyboard::Key::LControl, sf::Keyboard::Key::S) && SelectedChart)
				MOD(ChartParserModule).ExportChartSet(SelectedChart);

			MOD(ShortcutMenuModule).Separator();

			for (auto path : Config.RecentFilePaths)
				if (MOD(ShortcutMenuModule).MenuItem(path.c_str(), sf::Keyboard::Unknown, sf::Keyboard::Unknown))
					OpenChart(path);

			MOD(ShortcutMenuModule).EndMenu();
		}

		if (MOD(ShortcutMenuModule).BeginMenu("Edit"))
		{
			if (MOD(ShortcutMenuModule).MenuItem("Undo", sf::Keyboard::Key::LControl, sf::Keyboard::Key::Z) && SelectedChart && MOD(EditModule).OnUndo())
				PUSH_NOTIFICATION("Undo");

			if (MOD(ShortcutMenuModule).MenuItem("Redo", sf::Keyboard::Key::LControl, sf::Keyboard::Key::Y) && SelectedChart && MOD(EditModule).OnRedo())
				PUSH_NOTIFICATION("Redo");

			if (MOD(ShortcutMenuModule).MenuItem("Select All", sf::Keyboard::Key::LControl, sf::Keyboard::Key::A))
				MOD(EditModule).OnSelectAll();

            if (MOD(ShortcutMenuModule).MenuItem("Invert Selection", sf::Keyboard::Key::LControl, sf::Keyboard::Key::I))
                MOD(EditModule).OnInvertSelection();

			MOD(ShortcutMenuModule).Separator();

			if (MOD(ShortcutMenuModule).MenuItem("Copy", sf::Keyboard::Key::LControl, sf::Keyboard::Key::C) && SelectedChart)
				MOD(EditModule).OnCopy();

			if (MOD(ShortcutMenuModule).MenuItem("Paste", sf::Keyboard::Key::LControl, sf::Keyboard::Key::V) && SelectedChart)
				MOD(EditModule).OnPaste();

			if (MOD(ShortcutMenuModule).MenuItem("Delete", sf::Keyboard::Unknown, sf::Keyboard::Key::Delete) && SelectedChart)
				MOD(EditModule).OnDelete();

			MOD(ShortcutMenuModule).Separator();

			if (MOD(ShortcutMenuModule).MenuItem("Mirror", sf::Keyboard::Key::LControl, sf::Keyboard::Key::H) && SelectedChart)
				MOD(EditModule).OnMirror();

			if (MOD(ShortcutMenuModule).MenuItem("Stream Generator", sf::Keyboard::Key::Unknown, sf::Keyboard::Unknown) && SelectedChart)
				OpenStreamGenerator();

			if (MOD(ShortcutMenuModule).MenuItem("Difficulty Analyzer", sf::Keyboard::Key::Unknown, sf::Keyboard::Unknown) && SelectedChart)
				OpenDifficultyAnalyzer();

			if (MOD(ShortcutMenuModule).MenuItem("Reverse", sf::Keyboard::Key::LControl, sf::Keyboard::Key::R) && SelectedChart)
				MOD(EditModule).OnReverse();

			if (MOD(ShortcutMenuModule).MenuItem("Shuffle", sf::Keyboard::Key::LControl, sf::Keyboard::Key::J) && SelectedChart)
				MOD(EditModule).OnShuffle();

			if (MOD(ShortcutMenuModule).MenuItem("Quantize", sf::Keyboard::Key::LControl, sf::Keyboard::Key::Q) && SelectedChart)
				MOD(EditModule).OnQuantize(CurrentSnap);

            if (MOD(ShortcutMenuModule).MenuItem("Convert to Holds", sf::Keyboard::Key::Unknown, sf::Keyboard::Unknown) && SelectedChart)
            {
                // Calculate 1/4 length in ms based on current BPM
                // We need current time point to get BPM.
                Time currentTime = MOD(AudioModule).GetTimeMilliSeconds();
                BpmPoint* bpm = MOD(BeatModule).GetClosestBeatLineToTimePoint(currentTime).TimePoint > 0 ? SelectedChart->GetPreviousBpmPointFromTimePoint(currentTime) : nullptr;
                if (!bpm) bpm = SelectedChart->GetNextBpmPointFromTimePoint(-1000000);

                double length = 0;
                if (bpm) length = bpm->BeatLength; // 1 beat default? Or Snap?

                // Let's use CurrentSnap divisor
                if (CurrentSnap > 0 && bpm) length = bpm->BeatLength * (4.0 / CurrentSnap); // Snap is usually divisor of measure (48 = 1/48?)

                if (length <= 0) length = 500; // Fallback

                MOD(EditModule).OnConvertToHolds(Time(length));
            }

            if (MOD(ShortcutMenuModule).MenuItem("Convert to Taps", sf::Keyboard::Key::Unknown, sf::Keyboard::Unknown) && SelectedChart)
                MOD(EditModule).OnConvertToTaps();

            if (MOD(ShortcutMenuModule).MenuItem("Play Selection", sf::Keyboard::Key::LShift, sf::Keyboard::Key::Space) && SelectedChart)
            {
                 Time start, end;
                 if (MOD(EditModule).GetSelectionRange(start, end))
                 {
                     MOD(AudioModule).PlayRange(start, end);
                 }
            }

            // Using Ctrl+B for Estimate BPM (usually B is taken, but maybe Ctrl+Shift+B)
			if (MOD(ShortcutMenuModule).MenuItem("Estimate BPM", sf::Keyboard::Key::LControl, sf::Keyboard::Key::B) && SelectedChart)
				MOD(EditModule).OnEstimateBPM();

			if (MOD(ShortcutMenuModule).MenuItem("Tap BPM", sf::Keyboard::Key::LShift, sf::Keyboard::Key::T) && SelectedChart)
				MOD(EditModule).OnTap();

			if (MOD(ShortcutMenuModule).MenuItem("Expand", sf::Keyboard::Key::LControl, sf::Keyboard::Key::Up) && SelectedChart)
				MOD(EditModule).OnExpand();

			if (MOD(ShortcutMenuModule).MenuItem("Compress", sf::Keyboard::Key::LControl, sf::Keyboard::Key::Down) && SelectedChart)
				MOD(EditModule).OnCompress();

            // Snap to Peak: Ctrl+P
			if (MOD(ShortcutMenuModule).MenuItem("Snap to Peak", sf::Keyboard::Key::LControl, sf::Keyboard::Key::P) && SelectedChart)
				SnapToPeak();

			if (MOD(ShortcutMenuModule).MenuItem("Go To Timepoint", sf::Keyboard::Key::LControl, sf::Keyboard::Key::T) && SelectedChart)
				GoToTimePoint();

            if (MOD(ShortcutMenuModule).MenuItem("Move All Notes", sf::Keyboard::Key::Unknown, sf::Keyboard::Key::Unknown) && SelectedChart)
                OpenMoveAllNotes();

			MOD(ShortcutMenuModule).EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			ImGui::MenuItem("Dashboard Window", nullptr, &_ShowDashboard);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Options"))
		{
			std::string togglePitch = "Toggle Pitch (";
			togglePitch += Config.UsePitch ? "Pitched)" : "Stretched)";

			if(ImGui::MenuItem(togglePitch.c_str()))
			{
				Config.UsePitch = !Config.UsePitch;
				Config.Save();

				MOD(AudioModule).ResetSpeed();
				MOD(AudioModule).UsePitch = Config.UsePitch;

				PUSH_NOTIFICATION("Speed Reset");
			}

			ImGui::Separator();

			if(ImGui::MenuItem("Select Skin"))
			{
				MOD(DialogModule).OpenFolderDialog([this](const std::string &InPath)
				{
					Config.SkinFolderPath = InPath;
					Config.Save();

					if (SelectedChart){
						MOD(TimefieldRenderModule).InitializeResources(SelectedChart->KeyAmount, Config.SkinFolderPath);
					}

					PUSH_NOTIFICATION("Skin Changed");
				}, true);
			}

			ImGui::Separator();

			if(ImGui::Checkbox("Show Column Lines", &Config.ShowColumnLines))
			{
				MOD(TimefieldRenderModule).GetSkin().ShowColumnLines = Config.ShowColumnLines;
				Config.Save();
			}

			if (ImGui::Checkbox("Show Waveform", &Config.ShowWaveform)) Config.Save();

			if (ImGui::Checkbox("Use Auto Timing", &Config.UseAutoTiming))
			{
				EditMode::static_Flags.UseAutoTiming = Config.UseAutoTiming;
				Config.Save();
			}

			if (ImGui::Checkbox("Show Column Heatmap", &Config.ShowColumnHeatmap))
			{
				EditMode::static_Flags.ShowColumnHeatmap = Config.ShowColumnHeatmap;
				Config.Save();
			}

            ImGui::Checkbox("Metronome", &_MetronomeEnabled);
            ImGui::Checkbox("Assist Tick", &_AssistTickEnabled);

			ImGui::EndMenu();
		}

		if (MOD(ShortcutMenuModule).BeginMenu("Help"))
		{
			if (MOD(ShortcutMenuModule).MenuItem("Shortcuts", sf::Keyboard::Unknown, sf::Keyboard::F1))
				ShowShortCuts();

			MOD(ShortcutMenuModule).EndMenu();
		}

		if (ImGui::BeginMenu("Debug"))
		{
			ImGui::Checkbox("Show TimeSlice Boundaries", &MOD(DebugModule).ShowTimeSliceBoundaries);

			ImGui::EndMenu();
		}


		ImGui::EndMainMenuBar();
	}
}

void Program::SetUpMetadata()
{
	std::string titleName;

	if (ShouldSetUpNewChart)
		titleName = "New Chart";
	else
		titleName = "Edit Metadata";

	MOD(PopupModule).OpenPopup(titleName, [this](bool& OutOpen)
	{
			ImGui::Text("Meta Data");
			ImGui::InputText("Artist", &ChartMetadataSetup.Artist);
			ImGui::InputText("Song Title", &ChartMetadataSetup.SongTitle);
			ImGui::InputText("Charter", &ChartMetadataSetup.Charter);
			ImGui::InputText("Difficulty Name", &ChartMetadataSetup.DifficultyName);

			ImGui::NewLine();

			ImGui::Text("Difficulty");
			if (ShouldSetUpNewChart) ImGui::SliderInt("Key Amount", &ChartMetadataSetup.KeyAmount, 4, 10);
			else {
				std::string key = "Key = ";
				key.append(std::to_string(SelectedChart->KeyAmount));
				ImGui::Text(key.c_str());
			}

			ImGui::DragFloat("OD", &ChartMetadataSetup.OD, 0.1f, 1.0f, 10.0f);
			ImGui::DragFloat("HP", &ChartMetadataSetup.HP, 0.1f, 1.0f, 10.0f);

			ImGui::NewLine();

			std::string audioButtonName =  ChartMetadataSetup.AudioPath.string() == "" ? "Pick an audio file" : ChartMetadataSetup.AudioPath.string();
			std::string chartButtonName =  ChartMetadataSetup.ChartFolderPath.string() == "" ? "Pick a chart folder path" : ChartMetadataSetup.ChartFolderPath.string();
			std::string backgroundButtonName =  ChartMetadataSetup.BackgroundPath.string() == "" ? "Pick a background (optional)" : ChartMetadataSetup.BackgroundPath.string();

			ImGui::Text("Relevant Paths");

			if(ImGui::Button(audioButtonName.c_str()))
			{
				ShouldSetUpMetadata = OutOpen = false;

				MOD(DialogModule).OpenFileDialog(".mp3;.ogg;.wav", [this](const std::string &InPath)
				{
					ChartMetadataSetup.AudioPath = InPath;
					ShouldSetUpMetadata = true;
				}, true);
			}

			if(ImGui::Button(chartButtonName.c_str()))
			{
				ShouldSetUpMetadata = OutOpen = false;

				MOD(DialogModule).OpenFolderDialog([this](const std::string &InPath)
				{
					ChartMetadataSetup.ChartFolderPath = InPath;
					ShouldSetUpMetadata = true;
				}, true);
			}

			if(ImGui::Button(backgroundButtonName.c_str()))
			{
				ShouldSetUpMetadata = OutOpen = false;

				MOD(DialogModule).OpenFileDialog(".png;.jpg", [this](const std::string &InPath)
				{
					ChartMetadataSetup.BackgroundPath = InPath;
					ShouldSetUpMetadata = true;
				}, true);
			}

			ImGui::NewLine();

			if(ShouldSetUpNewChart)
			{
				if(ImGui::Button("Create"))
				{
					OpenChart(MOD(ChartParserModule).CreateNewChart(ChartMetadataSetup));

					ShouldSetUpMetadata = ShouldSetUpNewChart = OutOpen = false;
				}
			}

			else
			{
				if(ImGui::Button("Save"))
				{
					OpenChart(MOD(ChartParserModule).SetChartMetadata(SelectedChart, ChartMetadataSetup));

					ShouldSetUpMetadata = ShouldSetUpNewChart = OutOpen = false;
				}
			}

			ImGui::SameLine();

			if(ImGui::Button("Close"))
			{
				if (ShouldSetUpNewChart) ChartMetadataSetup = ChartMetadata();
				else ChartMetadataSetup = MOD(ChartParserModule).GetChartMetadata(SelectedChart);
				ShouldSetUpMetadata = ShouldSetUpNewChart = OutOpen = false;
			}
	});
}

void Program::ShowShortCuts()
{
	MOD(PopupModule).OpenPopup("New Chart", [this](bool& OutOpen)
	{
		MOD(ShortcutMenuModule).ShowCheatSheet();

		if(ImGui::Button("close"))
			OutOpen = false;
	});
}

void Program::OpenMoveAllNotes()
{
    static int offset = 0;
    MOD(PopupModule).OpenPopup("Move All Notes", [this](bool& OutOpen)
    {
        ImGui::InputInt("Offset (ms)", &offset);
        ImGui::Text("Positive = Move Right (Later)");
        ImGui::Text("Negative = Move Left (Earlier)");

        if(ImGui::Button("Apply"))
        {
            if (offset != 0)
            {
                MOD(EditModule).OnMoveAllNotes(offset);
                MOD(BeatModule).AssignNotesToSnapsInChart(SelectedChart);
                PUSH_NOTIFICATION("Moved all notes by %d ms", offset);
            }
            OutOpen = false;
        }

        ImGui::SameLine();

        if(ImGui::Button("Cancel") || MOD(InputModule).WasKeyPressed(sf::Keyboard::Key::Escape))
            OutOpen = false;
    });
}

void Program::OpenStreamGenerator()
{
	static int start = 0;
	static int end = 0;
	static int divisor = 16;
	static int pattern = 0;

	if (start == 0 && end == 0)
	{
		start = WindowTimeBegin;
		end = WindowTimeEnd;
	}

	MOD(PopupModule).OpenPopup("Stream Generator", [this](bool& OutOpen)
	{
		ImGui::InputInt("Start Time (ms)", &start);
		ImGui::InputInt("End Time (ms)", &end);

		const char* patterns[] = { "Staircase", "Trill", "Spiral", "Random", "Jumpstream", "Handstream", "Chordjack" };
		ImGui::Combo("Pattern", &pattern, patterns, IM_ARRAYSIZE(patterns));

		ImGui::InputInt("Snap (4, 8, 16...)", &divisor);
		if (divisor < 1) divisor = 1;

		if(ImGui::Button("Generate"))
		{
			SelectedChart->GenerateStream(start, end, divisor, (StreamPattern)pattern);
            MOD(BeatModule).RecalculateSnaps(SelectedChart, start, end);
			OutOpen = false;
		}

		ImGui::SameLine();

		if(ImGui::Button("Cancel") || MOD(InputModule).WasKeyPressed(sf::Keyboard::Key::Escape))
			OutOpen = false;
	});
}

void Program::OpenDifficultyAnalyzer()
{
	MOD(PopupModule).OpenPopup("Difficulty Analyzer", [this](bool& OutOpen)
	{
		static int windowSize = 1000;
		ImGui::InputInt("Window Size (ms)", &windowSize);
		if (windowSize < 100) windowSize = 100;

		std::vector<float> graph = SelectedChart->CalculateNPSGraph(windowSize);
		if (!graph.empty())
		{
			ImGui::PlotLines("NPS", graph.data(), graph.size(), 0, NULL, 0.0f, FLT_MAX, ImVec2(0, 100));
		}

		float avg = SelectedChart->GetAverageNPS();
		float peak = SelectedChart->GetPeakNPS();

		ImGui::Text("Average NPS: %.2f", avg);
		ImGui::Text("Peak NPS (1s): %.2f", peak);

		if(ImGui::Button("Close") || MOD(InputModule).WasKeyPressed(sf::Keyboard::Key::Escape))
			OutOpen = false;
	});
}

void Program::SnapToPeak()
{
    Time current = MOD(AudioModule).GetTimeMilliSeconds();
    Time peak = MOD(AudioModule).FindNearestPeak(current, 50); // +/- 50ms window
    MOD(AudioModule).SetTimeMilliSeconds(peak);
    PUSH_NOTIFICATION("Snapped to Peak: %d -> %d", current, peak);
}

void Program::GoToTimePoint()
{
	MOD(PopupModule).OpenPopup("Go To Timepoint", [this](bool& OutOpen)
	{
		ImGui::InputText("Timepoint (MS)", &TimeToGo);

		if(ImGui::Button("Go") || MOD(InputModule).WasKeyPressed(sf::Keyboard::Key::Enter))
		{
			MOD(AudioModule).SetTimeMilliSeconds(std::stoi(TimeToGo));

			OutOpen = false;
		}

		ImGui::SameLine();

		if(ImGui::Button("Cancel") || MOD(InputModule).WasKeyPressed(sf::Keyboard::Key::Escape))
			OutOpen = false;
	});
}

void Program::InputActions()
{
	MOD(EditModule).SetShiftKeyState(MOD(InputModule).IsShiftKeyDown());

	if (MOD(InputModule).IsTogglingPause())
    {
        if (MOD(InputModule).IsShiftKeyDown())
        {
             Time start, end;
             if (MOD(EditModule).GetSelectionRange(start, end))
             {
                 MOD(AudioModule).PlayRange(start, end);
                 return;
             }
        }
		MOD(AudioModule).TogglePause();
    }

	if (MOD(InputModule).WasKeyPressed(sf::Keyboard::Key::Num1))
	{
		MOD(EditModule).SetEditMode<SelectEditMode>();
		PUSH_NOTIFICATION("Select Edit Mode");
	}

	if (MOD(InputModule).WasKeyPressed(sf::Keyboard::Key::Num2))
	{
		MOD(EditModule).SetEditMode<NoteEditMode>();
		PUSH_NOTIFICATION("Note Edit Mode");
	}

	if (MOD(InputModule).WasKeyPressed(sf::Keyboard::Key::Num3))
	{
		MOD(EditModule).SetEditMode<BpmEditMode>();
		PUSH_NOTIFICATION("Bpm Edit Mode");
	}

	if (MOD(InputModule).IsCtrlKeyDown())
	{
		if (MOD(InputModule).IsScrollingUp())
			return ApplyDeltaToZoom(1.0f);

		if (MOD(InputModule).IsScrollingDown())
			return ApplyDeltaToZoom(-1.0f);
	}

	if (MOD(InputModule).IsAltKeyDown())
	{
		if (MOD(InputModule).IsScrollingUp())
			return void(CurrentSnap = MOD(BeatModule).GetNextSnap(CurrentSnap)), PUSH_NOTIFICATION_LIFETIME(0.5f, "Snap %d", CurrentSnap);

		if (MOD(InputModule).IsScrollingDown())
			return void(CurrentSnap = MOD(BeatModule).GetPreviousSnap(CurrentSnap)), PUSH_NOTIFICATION_LIFETIME(0.5f, "Snap %d", CurrentSnap);
	}

	if (MOD(InputModule).IsShiftKeyDown())
	{
		if (MOD(InputModule).IsScrollingUp() || MOD(InputModule).IsRightKeyPressed())
			return MOD(AudioModule).ChangeSpeed(0.05f), PUSH_NOTIFICATION_LIFETIME(0.5f, "Speed %.2fx", MOD(AudioModule).GetPlaybackSpeed());

		if (MOD(InputModule).IsScrollingDown() || MOD(InputModule).IsLeftKeyPressed())
			return MOD(AudioModule).ChangeSpeed(-0.05f), PUSH_NOTIFICATION_LIFETIME(0.5f, "Speed %.2fx", MOD(AudioModule).GetPlaybackSpeed());
	}

	if (MOD(InputModule).IsScrollingUp() || MOD(InputModule).IsDownKeyPressed())
		return MOD(AudioModule).SetTimeMilliSeconds(MOD(BeatModule).GetPreviousBeatLine(MOD(AudioModule).GetTimeMilliSeconds()).TimePoint);

	if (MOD(InputModule).IsScrollingDown() || MOD(InputModule).IsUpKeyPressed())
		return MOD(AudioModule).SetTimeMilliSeconds(MOD(BeatModule).GetNextBeatLine(MOD(AudioModule).GetTimeMilliSeconds()).TimePoint);

	if (ImGui::GetIO().WantCaptureMouse)
		return;

	if (MOD(MiniMapModule).IsDragging())
	{
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			return MOD(MiniMapModule).EndDragging();

		MOD(AudioModule).SetTimeMilliSeconds((MOD(MiniMapModule).GetHoveredTime()));
	}

	if (MOD(MiniMapModule).IsHoveringTimeline(32, _WindowMetrics.MenuBarHeight, _WindowMetrics.Height - _WindowMetrics.MenuBarHeight, 16, MOD(AudioModule).GetTimeMilliSeconds(), WindowTimeBegin, WindowTimeEnd, EditCursor))
	{
		if (MOD(MiniMapModule).IsPossibleToDrag())
		{
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				return MOD(MiniMapModule).StartDragging();
		}
		else if (!MOD(MiniMapModule).IsDragging())
		{
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				return MOD(AudioModule).SetTimeMilliSeconds((MOD(MiniMapModule).GetHoveredTime()));
		}

		return;
	}

	if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		return void(MOD(EditModule).OnMouseLeftButtonReleased());

	if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
		return void(MOD(EditModule).OnMouseRightButtonReleased());

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		return void(MOD(EditModule).OnMouseLeftButtonClicked(MOD(InputModule).IsShiftKeyDown()));

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		return void(MOD(EditModule).OnMouseRightButtonClicked(MOD(InputModule).IsShiftKeyDown()));
}

void Program::InitializeChart(Chart* InChart)
{
    SelectedChart = InChart;

	MOD(BeatModule).AssignNotesToSnapsInChart(SelectedChart);
	MOD(AudioModule).LoadAudio(SelectedChart->AudioPath);
	MOD(EditModule).SetChart(SelectedChart);
	MOD(BackgroundModule).LoadBackground(SelectedChart->BackgroundPath);
	MOD(TimefieldRenderModule).InitializeResources(SelectedChart->KeyAmount, Config.SkinFolderPath);
	MOD(MiniMapModule).Generate(SelectedChart, MOD(TimefieldRenderModule).GetSkin(), MOD(AudioModule).GetSongLengthMilliSeconds());
	MOD(WaveFormModule).SetWaveFormData(MOD(AudioModule).GenerateAndGetWaveformData(SelectedChart->AudioPath), MOD(AudioModule).GetSongLengthMilliSeconds());
	ChartMetadataSetup = MOD(ChartParserModule).GetChartMetadata(SelectedChart);

	SelectedChart->RegisterOnModifiedCallback([this](TimeSlice &InTimeSlice)
	{
		//TODO: replicate the timeslice method to optimize when "re-generating"
		MOD(BeatModule).AssignNotesToSnapsInTimeSlice(SelectedChart, InTimeSlice);
		MOD(MiniMapModule).GeneratePortion(InTimeSlice, MOD(TimefieldRenderModule).GetSkin());
	});
}

void Program::OpenChart(const std::string& InPath)
{
    auto charts = MOD(ChartParserModule).ScanForCharts(InPath);

    if (charts.empty())
    {
		PUSH_NOTIFICATION("File not found! It might have been deleted?");
		Config.DeleteRecentFile(InPath);
		Config.Save();
		return;
    }

    if (charts.size() == 1)
    {
        Chart* chart = MOD(ChartParserModule).LoadChart(InPath, charts[0].DifficultyName);
        if (chart)
        {
            Config.RegisterRecentFile(InPath);
            Config.Save();
            InitializeChart(chart);
        }
        else
        {
            PUSH_NOTIFICATION("Failed to load chart");
        }
    }
    else
    {
        MOD(PopupModule).OpenPopup("Select Difficulty", [this, charts, InPath](bool& OutOpen) mutable
        {
            ImGui::Text("Select a chart to load:");
            for (const auto& def : charts)
            {
                std::string label = def.DifficultyName;
                if (label.empty()) label = "Untitled";
                if (!def.ChartType.empty()) label += " (" + def.ChartType + ")";

                if (ImGui::Button(label.c_str(), ImVec2(-1, 0)))
                {
                    Chart* chart = MOD(ChartParserModule).LoadChart(InPath, def.DifficultyName);
                    if (chart)
                    {
                        Config.RegisterRecentFile(InPath);
                        Config.Save();
                        InitializeChart(chart);
                    }
                    else
                    {
                        PUSH_NOTIFICATION("Failed to load chart");
                    }
                    OutOpen = false;
                }
            }
            if (ImGui::Button("Cancel", ImVec2(-1, 0))) OutOpen = false;
        });
    }
}

void Program::ApplyDeltaToZoom(const float InDelta)
{
	if(InDelta > 0)
		ZoomIndex++;
	else
		ZoomIndex--;

	if(ZoomIndex > int(LegalZoomLevels.size() - 1))
		ZoomIndex =	LegalZoomLevels.size() - 1;

	if(ZoomIndex < 0)
		ZoomIndex = 0;

	ZoomLevel = LegalZoomLevels[ZoomIndex];
}

void Program::UpdateCursor()
{
	//TODO: module? get dependencies and throw it into a module
	if (ImGui::GetIO().WantCaptureMouse)
		return;

	const auto &timefieldMetrics = MOD(TimefieldRenderModule).GetTimefieldMetrics();

	EditCursor.X = (int)ImGui::GetMousePos().x;
	EditCursor.Y = (int)ImGui::GetMousePos().y;

	BeatLine snappedBeatLine = MOD(BeatModule).GetCurrentBeatLine(MOD(TimefieldRenderModule).GetTimeFromScreenPoint(EditCursor.Y, MOD(AudioModule).GetTimeMilliSeconds(), ZoomLevel, true), 1);
	EditCursor.TimeFieldY = MOD(TimefieldRenderModule).GetScreenPointFromTime(snappedBeatLine.TimePoint, MOD(AudioModule).GetTimeMilliSeconds(), ZoomLevel);

	if (EditCursor.TimeFieldY > _WindowMetrics.Height)
	{
		snappedBeatLine = MOD(BeatModule).GetCurrentBeatLine(MOD(TimefieldRenderModule).GetTimeFromScreenPoint(EditCursor.Y, MOD(AudioModule).GetTimeMilliSeconds(), ZoomLevel, true), 0, false);
		EditCursor.TimeFieldY = MOD(TimefieldRenderModule).GetScreenPointFromTime(snappedBeatLine.TimePoint, MOD(AudioModule).GetTimeMilliSeconds(), ZoomLevel);
	}

	EditCursor.TimePoint = snappedBeatLine.TimePoint;
	EditCursor.UnsnappedTimePoint = MOD(TimefieldRenderModule).GetTimeFromScreenPoint(EditCursor.Y, MOD(AudioModule).GetTimeMilliSeconds(), ZoomLevel);
	EditCursor.CursorColumn = MOD(TimefieldRenderModule).GetColumnFromScreenPoint(EditCursor.X);
	EditCursor.BeatSnap = snappedBeatLine.BeatSnap;

	if (EditCursor.X < timefieldMetrics.LeftSidePosition)
		EditCursor.TimefieldSide = Cursor::FieldPosition::Left;
	else if (EditCursor.X > timefieldMetrics.LeftSidePosition + timefieldMetrics.FieldWidth)
		EditCursor.TimefieldSide = Cursor::FieldPosition::Right;
	else
		EditCursor.TimefieldSide = Cursor::FieldPosition::Middle;

	EditCursor.HoveredNotes.clear();

	MOD(TimefieldRenderModule).GetOverlappedOnScreenNotes(EditCursor.CursorColumn, EditCursor.Y, EditCursor.HoveredNotes);

	std::sort(EditCursor.HoveredNotes.begin(), EditCursor.HoveredNotes.end(), [](const Note *lhs, const Note *rhs) { return lhs->TimePoint < rhs->TimePoint; });
}

void Program::SetConfig(const Configuration& InConfig)
{
	MOD(AudioModule).UsePitch = Config.UsePitch;
	MOD(TimefieldRenderModule).GetSkin().ShowColumnLines = Config.ShowColumnLines;
	EditMode::static_Flags.UseAutoTiming = Config.UseAutoTiming;
	EditMode::static_Flags.ShowColumnHeatmap = Config.ShowColumnHeatmap;
}