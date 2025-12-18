#include "bpm-edit-mode.h"
#include "imgui.h"

#include "../modules/manager/module-manager.h"
#include "../modules/audio-module.h"


bool BpmEditMode::OnMouseLeftButtonClicked(const bool InIsShiftDown)
{
    if(static_Cursor.TimefieldSide != Cursor::FieldPosition::Middle)
        return false;

    if(_HoveredBpmPoint != nullptr)
    {
        _MovableBpmPoint = _HoveredBpmPoint;
        _MovableBpmPointInitialValue = *_MovableBpmPoint;

        static_Chart->RegisterTimeSliceHistory(_MovableBpmPoint->TimePoint);

        _PreviousBpmPoint = static_Chart->GetPreviousBpmPointFromTimePoint(_MovableBpmPoint->TimePoint);
        _NextBpmPoint = static_Chart->GetNextBpmPointFromTimePoint(_MovableBpmPoint->TimePoint);

        return false;
    }

    if(static_Flags.UseAutoTiming)
        PlaceAutoTimePoint();
    else
        PlaceTimePoint();


    return true;
}

bool BpmEditMode::OnMouseLeftButtonReleased()
{
    if(_MovableBpmPoint != nullptr)
    {
        static_Chart->RevaluateBpmPoint(_MovableBpmPointInitialValue, *_MovableBpmPoint);

        _MovableBpmPoint = nullptr;
        return false;
    }

    return false;
}

bool BpmEditMode::OnMouseRightButtonClicked(const bool InIsShiftDown)
{
    if(_HoveredBpmPoint && !_MovableBpmPoint)
    {
        if(static_Flags.UseAutoTiming)
        {
            if(BpmPoint* previousBpmPoint = static_Chart->GetPreviousBpmPointFromTimePoint(_HoveredBpmPoint->TimePoint))
            {
                if(BpmPoint* nextBpmPoint = static_Chart->GetNextBpmPointFromTimePoint(_HoveredBpmPoint->TimePoint))
                {
                    Time deltaTime = abs((previousBpmPoint->TimePoint) - nextBpmPoint->TimePoint);

                    double beatLength = double(deltaTime);
                    double newBpm = 60000.0 / beatLength;

                    previousBpmPoint->BeatLength = beatLength;
                    previousBpmPoint->Bpm = newBpm;
                }
            }
        }

        static_Chart->RemoveBpmPoint(*_HoveredBpmPoint);
        _HoveredBpmPoint = nullptr;

        _VisibleBpmPoints->clear();
    }

    return false;
}

void BpmEditMode::OnEstimateBPM()
{
    Time length = MOD(AudioModule).GetSongLengthMilliSeconds();
    float bpm = MOD(AudioModule).EstimateBPM(0, length);

    if (bpm > 0.0f)
    {
        // Estimate Offset
        Time offset = MOD(AudioModule).EstimateOffset(bpm, 0, length);

        PUSH_NOTIFICATION("Estimated BPM: %.2f, Offset: %d", bpm, offset);

        if (_MovableBpmPoint)
        {
             _MovableBpmPoint->Bpm = bpm;
             _MovableBpmPoint->BeatLength = 60000.0 / bpm;
             _MovableBpmPoint->TimePoint = offset;
        }
        else
        {
            // If no point hovered, maybe place one at offset?
            // For now, let's just notify. The user might want to drag a point there.
            // Or we could snap the cursor?
            // MOD(AudioModule).SetTimeMilliSeconds(offset); // Auto-seek to offset
        }
    }
    else
    {
        PUSH_NOTIFICATION("BPM Estimation Failed");
    }
}

void BpmEditMode::OnTap()
{
    // Need a reliable clock. AudioModule has GetTimeMilliSeconds() but that depends on song position which might be paused.
    // We want real-time taps.
    // SFML Clock would be ideal, or std::chrono.
    // AudioModule has BASS, BASS has BASS_ChannelBytes2Seconds(BASS_ChannelGetPosition(..)) which is song time.
    // If song is playing, song time is fine. If paused, it's not.
    // Tapping is usually done while listening to the song.

    // Logic:
    // 1. Get current time (either song time or system time).
    //    If we use song time, we can handle playback rate changes automatically if we tap to the heard beat.
    //    But if we seek, taps become invalid.

    // Let's use std::chrono::steady_clock for raw interval calculation.
    // But then we need to know the playback rate to convert to BPM? No, if we tap to the beat, the interval is real-time interval.
    // If playback is 1.0x, BPM = 60 / interval.
    // If playback is 2.0x, we tap twice as fast. interval is half. 60/interval is 2*BPM.
    // So calculated BPM is playback rate * true BPM? No.
    // If song is 120 BPM.
    // 1.0x: Beat every 0.5s. Tap interval 0.5s. 60/0.5 = 120. Correct.
    // 2.0x: Beat every 0.25s. Tap interval 0.25s. 60/0.25 = 240. Correct (we are hearing 240 BPM).
    // But we want the song BPM. So we must divide by playback speed.

    using namespace std::chrono;
    static auto lastTapTime = steady_clock::now();
    auto now = steady_clock::now();

    // Reset if too long since last tap (e.g. 2 seconds)
    if (duration_cast<milliseconds>(now - lastTapTime).count() > 2000)
    {
        _TapTimes.clear();
        _TappedBPM = 0;
        PUSH_NOTIFICATION("Tapper Reset");
    }

    _TapTimes.push_back(duration_cast<milliseconds>(now.time_since_epoch()).count());
    lastTapTime = now;

    if (_TapTimes.size() > 1)
    {
        // Calculate average interval
        double sumIntervals = 0;
        for (size_t i = 1; i < _TapTimes.size(); ++i)
        {
            sumIntervals += (_TapTimes[i] - _TapTimes[i-1]);
        }
        double avgInterval = sumIntervals / (_TapTimes.size() - 1);

        // Convert to seconds
        double intervalSec = avgInterval / 1000.0;

        // Adjust for playback speed
        float speed = MOD(AudioModule).GetPlaybackSpeed();
        if (speed <= 0.001f) speed = 1.0f;

        double bpm = (60.0 / intervalSec) / speed;
        _TappedBPM = float(bpm);

        PUSH_NOTIFICATION("Tapped BPM: %.2f", _TappedBPM);

        // If we have a movable point, update it?
        // Or if we are hovering.
        if (_HoveredBpmPoint)
        {
             // Update hovered BPM? Maybe risky.
             // ArrowVortex usually has a separate "Tap" window.
             // Let's just notify for now, user can manually input.
             // Or update _MovableBpmPoint if dragging.
        }
    }
}

void BpmEditMode::SubmitToRenderGraph(TimefieldRenderGraph& InOutTimefieldRenderGraph, const Time InTimeBegin, const Time InTimeEnd)
{
    if(_PinnedBpmPoint)
    {
        InOutTimefieldRenderGraph.SubmitTimefieldRenderCommand(0, _PinnedBpmPoint->TimePoint,
        [this](sf::RenderTarget* const InRenderTarget, const TimefieldMetrics& InTimefieldMetrics, const int InScreenX, const int InScreenY)
        {
            float posY = InRenderTarget->getView().getSize().y / 2.f;
            float posX = InTimefieldMetrics.LeftSidePosition + InTimefieldMetrics.FieldWidth + 64.f;

            sf::VertexArray line(sf::Lines, 2);
            line[0].position.x = InTimefieldMetrics.LeftSidePosition + InTimefieldMetrics.FieldWidth;
            line[0].position.y = InScreenY;
            line[0].color = sf::Color(255, 255, 255, 255);

            line[1].position.x = posX;
            line[1].position.y = posY;
            line[1].color = sf::Color(255, 255, 255, 255);

            InRenderTarget->draw(line);

            DisplayBpmNode(*_PinnedBpmPoint, posX , posY, true);
        });
    }

    //a bit hacky but if it works and is isolated it works for now I guess
	_VisibleBpmPoints = &(static_Chart->GetBpmPointsRelatedToTimeRange(InTimeBegin, InTimeEnd));

	for (auto& bpmPointPtr : *_VisibleBpmPoints)
	{
        InOutTimefieldRenderGraph.SubmitTimefieldRenderCommand(0, bpmPointPtr->TimePoint,
        [this, bpmPointPtr](sf::RenderTarget* const InRenderTarget, const TimefieldMetrics& InTimefieldMetrics, const int InScreenX, const int InScreenY)
        {
            sf::RectangleShape bpmLine;

            if(_HoveredBpmPoint == bpmPointPtr)
            {
                bpmLine.setPosition(InTimefieldMetrics.LeftSidePosition, InScreenY - 8);
                bpmLine.setSize(sf::Vector2f(InTimefieldMetrics.FieldWidth, 8));
                bpmLine.setFillColor(sf::Color(128, 255, 128, 255));
            }
            else
            {
                bpmLine.setPosition(InTimefieldMetrics.LeftSidePosition, InScreenY - 2);
                bpmLine.setSize(sf::Vector2f(InTimefieldMetrics.FieldWidth, 4));
                bpmLine.setFillColor(sf::Color(255, 255, 255, 255));
            }

            InRenderTarget->draw(bpmLine);

            if(bpmPointPtr == _PinnedBpmPoint)
                return;

            sf::RectangleShape indicator;
            indicator.setPosition(InTimefieldMetrics.LeftSidePosition + InTimefieldMetrics.FieldWidth, InScreenY);
            indicator.setSize(sf::Vector2f(16, 1));
            indicator.setFillColor(sf::Color(255, 255, 255, 255));

            InRenderTarget->draw(indicator);

            //god is dead
            DisplayBpmNode(*bpmPointPtr, InTimefieldMetrics.LeftSidePosition + InTimefieldMetrics.FieldWidth + 8, InScreenY);
        });
    }

    if(static_Cursor.TimefieldSide != Cursor::FieldPosition::Middle || _HoveredBpmPoint != nullptr)
        return;

    InOutTimefieldRenderGraph.SubmitTimefieldRenderCommand(0, GetCursorTime(),
    [this](sf::RenderTarget* const InRenderTarget, const TimefieldMetrics& InTimefieldMetrics, const int InScreenX, const int InScreenY)
    {
        sf::RectangleShape rectangle;

        rectangle.setPosition(InTimefieldMetrics.LeftSidePosition, InScreenY);
        rectangle.setSize(sf::Vector2f(InTimefieldMetrics.FieldWidth, 4));

        rectangle.setFillColor(sf::Color(255, 255, 255, 255));

        InRenderTarget->draw(rectangle);
    });
}

void BpmEditMode::Tick()
{
    if(_MovableBpmPoint)
    {
        _MovableBpmPoint->TimePoint = GetCursorTime();

        if(!static_Flags.UseAutoTiming)
            return;

        if(_PreviousBpmPoint)
        {
            Time deltaTime = abs((_PreviousBpmPoint->TimePoint) - _MovableBpmPoint->TimePoint);

            double beatLength = double(deltaTime);
            double newBpm = 60000.0 / beatLength;

            _PreviousBpmPoint->BeatLength = beatLength;
            _PreviousBpmPoint->Bpm = newBpm;
        }

        if(_NextBpmPoint)
        {
            Time deltaTime = abs(_NextBpmPoint->TimePoint - _MovableBpmPoint->TimePoint);

            double beatLength = double(deltaTime);
            double newBpm = 60000.0 / beatLength;

            _MovableBpmPoint->BeatLength = beatLength;
            _MovableBpmPoint->Bpm = newBpm;
        }

        return;
    }

    for (auto& bpmPointPtr : *_VisibleBpmPoints)
	{
        if(abs(GetCursorTime() - bpmPointPtr->TimePoint) < 20 && static_Cursor.TimefieldSide == Cursor::FieldPosition::Middle)
            return void(_HoveredBpmPoint = bpmPointPtr);
    }

    _HoveredBpmPoint = nullptr;
}

void BpmEditMode::PlaceAutoTimePoint()
{
    Time cursorTime = GetCursorTime() ;
    BpmPoint* placedBpmPoint = nullptr;

    if(BpmPoint* previousBpmPoint = static_Chart->GetPreviousBpmPointFromTimePoint(GetCursorTime()))
    {
        Time deltaTime = abs(previousBpmPoint->TimePoint - cursorTime);

        double beatLength = double(deltaTime);
        double newBpm = 60000.0 / beatLength;

        previousBpmPoint->BeatLength = beatLength;
        previousBpmPoint->Bpm = newBpm;
    }

    if(BpmPoint* nextBpmPoint =  static_Chart->GetNextBpmPointFromTimePoint(cursorTime))
    {
        Time deltaTime = abs(nextBpmPoint->TimePoint - cursorTime);

        double beatLength = double(deltaTime);
        double newBpm = 60000.0 / beatLength;

        static_Chart->PlaceBpmPoint(cursorTime, newBpm, beatLength);
    }
    else
        static_Chart->PlaceBpmPoint(cursorTime, 120.0, 60000.0 / 120.0);
}

void BpmEditMode::PlaceTimePoint()
{
    BpmPoint* previousBpmPoint = static_Chart->GetPreviousBpmPointFromTimePoint(GetCursorTime() );

    if(previousBpmPoint)
        static_Chart->PlaceBpmPoint(GetCursorTime(), previousBpmPoint->Bpm, previousBpmPoint->BeatLength);
    else
        static_Chart->PlaceBpmPoint(GetCursorTime(), 120.0, 60000.0 / 120.0);
}


void BpmEditMode::DisplayBpmNode(BpmPoint& InBpmPoint, const int InScreenX, const int InScreenY, const bool InIsPinned)
{
    ImGuiWindowFlags windowFlags = 0;
	windowFlags |= ImGuiWindowFlags_NoTitleBar;
	windowFlags |= ImGuiWindowFlags_NoMove;
	windowFlags |= ImGuiWindowFlags_NoResize;
	windowFlags |= ImGuiWindowFlags_NoCollapse;
	windowFlags |= ImGuiWindowFlags_AlwaysAutoResize;
	windowFlags |= ImGuiWindowFlags_NoScrollbar;

	bool open = true;

	ImGui::SetNextWindowPos({ float(InScreenX), float(InScreenY) });
	ImGui::Begin(std::to_string(reinterpret_cast<intptr_t>(&InBpmPoint)).c_str(), &open, windowFlags);

    if(InIsPinned)
    {
        if(ImGui::Button("Unpin"))
            _PinnedBpmPoint = nullptr;
    }
    else
    {
	    if(ImGui::Button("Pin"))
            _PinnedBpmPoint = &InBpmPoint;
    }

	ImGui::SameLine();

	ImGui::Text("BPM");
	ImGui::SameLine();
	ImGui::PushItemWidth(96);

    float bpmFloat = float(InBpmPoint.Bpm);
	ImGui::DragFloat(" ", &bpmFloat, 0.1f, 0.01f, 2000.0f);
	InBpmPoint.Bpm = double(bpmFloat);
    InBpmPoint.BeatLength = 60000.0 / InBpmPoint.Bpm;

    if(InIsPinned)
    {
        if(ImGui::Button("+1 MS"))
            InBpmPoint.TimePoint++;

        ImGui::SameLine();

        if(ImGui::Button("-1 MS"))
            InBpmPoint.TimePoint--;
    }

	ImGui::End();
}

Time BpmEditMode::GetCursorTime()
{
    return static_ShiftKeyState ? static_Cursor.TimePoint : static_Cursor.UnsnappedTimePoint;
}