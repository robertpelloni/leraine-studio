#include <iostream>
#include <cassert>
#include <vector>
#include <cmath>

#include "../source/structures/chart.h"
#include "../source/modules/chart-parser-module.h"
#include "../source/modules/shortcut-menu-module.h"
#include "../source/structures/skin.h"
#include "../source/modules/waveform-module.h"
#include "../source/modules/audio-module.h"
#include "../source/structures/configuration.h"

// Simple test framework
#define ASSERT(cond) if(!(cond)) { std::cerr << "Assertion failed: " << #cond << std::endl; return 1; }
#define TEST(name) std::cout << "Running " << #name << "..." << std::endl; if(name() != 0) { std::cerr << #name << " FAILED" << std::endl; return 1; } else { std::cout << #name << " PASSED" << std::endl; }

int TestChartLogic() {
    Chart chart;
    chart.KeyAmount = 4;

    // Inject notes
    chart.InjectNote(1000, 0, Note::EType::Common);
    chart.InjectNote(2000, 1, Note::EType::Common);

    // Select them
    NoteReferenceCollection selection;
    chart.FillNoteCollectionWithAllNotes(selection);

    ASSERT(selection.NoteAmount == 2);

    // Reverse
    chart.ReverseNotes(selection);

    // 1000 -> 2000, 2000 -> 1000
    Note* n1 = chart.FindNote(2000, 0);
    Note* n2 = chart.FindNote(1000, 1);

    ASSERT(n1 != nullptr);
    ASSERT(n2 != nullptr);

    return 0;
}

int TestWaveformCenterOffset()
{
    WaveFormModule waveform;
    ASSERT(waveform.GetCenterOffset() == 0);

    waveform.SetCenterOffset(50);
    ASSERT(waveform.GetCenterOffset() == 50);

    // Test bounds clamping [-128, 128]
    waveform.SetCenterOffset(200);
    ASSERT(waveform.GetCenterOffset() == 128);

    waveform.SetCenterOffset(-300);
    ASSERT(waveform.GetCenterOffset() == -128);

    Configuration config;
    ASSERT(config.WaveformCenterOffset == 0);
    config.WaveformCenterOffset = 25;
    ASSERT(config.WaveformCenterOffset == 25);

    return 0;
}

int TestNoteCountForColumn()
{
    Chart chart;
    chart.KeyAmount = 4;

    chart.InjectNote(1000, 0, Note::EType::Common);
    chart.InjectNote(1500, 0, Note::EType::Common);
    chart.InjectNote(2000, 1, Note::EType::Common);
    chart.InjectNote(2500, 3, Note::EType::Common);

    ASSERT(chart.GetNoteCountForColumn(0) == 2);
    ASSERT(chart.GetNoteCountForColumn(1) == 1);
    ASSERT(chart.GetNoteCountForColumn(2) == 0);
    ASSERT(chart.GetNoteCountForColumn(3) == 1);
    // Out of bounds check
    ASSERT(chart.GetNoteCountForColumn(4) == 0);
    ASSERT(chart.GetNoteCountForColumn(99) == 0);

    return 0;
}

int TestChartClampNotesToTimeRange()
{
    Chart chart;
    chart.KeyAmount = 4;

    chart.InjectNote(500, 0, Note::EType::Common);
    chart.InjectNote(1500, 1, Note::EType::Common);
    chart.InjectNote(2500, 2, Note::EType::Common);
    chart.InjectNote(3500, 3, Note::EType::Common);

    // Clamp to [1000, 3000] -> removes 500 and 3500
    chart.ClampNotesToTimeRange(1000, 3000);

    ASSERT(chart.FindNote(500, 0) == nullptr);
    ASSERT(chart.FindNote(3500, 3) == nullptr);
    ASSERT(chart.FindNote(1500, 1) != nullptr);
    ASSERT(chart.FindNote(2500, 2) != nullptr);

    return 0;
}

int TestWaveformWidthScale()
{
    WaveFormModule waveform;
    ASSERT(waveform.GetWidthScale() == 1.0f);

    waveform.SetWidthScale(1.5f);
    ASSERT(waveform.GetWidthScale() == 1.5f);

    // Test bounds clamping [0.5, 2.0]
    waveform.SetWidthScale(5.0f);
    ASSERT(waveform.GetWidthScale() == 2.0f);

    waveform.SetWidthScale(0.1f);
    ASSERT(waveform.GetWidthScale() == 0.5f);

    Configuration config;
    ASSERT(config.WaveformWidthScale == 1.0f);
    config.WaveformWidthScale = 1.25f;
    ASSERT(config.WaveformWidthScale == 1.25f);

    return 0;
}

int TestChartShiftNotesByColumn()
{
    Chart chart;
    chart.KeyAmount = 4;

    chart.InjectNote(1000, 1, Note::EType::Common);
    chart.InjectNote(2000, 2, Note::EType::Common);

    // Shift right (+1)
    int shiftedRight = chart.ShiftNotesByColumn(1);
    ASSERT(shiftedRight == 2);
    ASSERT(chart.FindNote(1000, 1) == nullptr);
    ASSERT(chart.FindNote(1000, 2) != nullptr);
    ASSERT(chart.FindNote(2000, 2) == nullptr);
    ASSERT(chart.FindNote(2000, 3) != nullptr);

    // Shift left (-2) -> col 2->0, col 3->1
    int shiftedLeft = chart.ShiftNotesByColumn(-2);
    ASSERT(shiftedLeft == 2);
    ASSERT(chart.FindNote(1000, 0) != nullptr);
    ASSERT(chart.FindNote(2000, 1) != nullptr);

    return 0;
}

int TestWaveformInversion()
{
    WaveFormModule waveform;
    ASSERT(!waveform.IsInverted());

    waveform.SetInverted(true);
    ASSERT(waveform.IsInverted());

    waveform.SetInverted(false);
    ASSERT(!waveform.IsInverted());

    Configuration config;
    ASSERT(!config.InvertWaveform);
    config.InvertWaveform = true;
    ASSERT(config.InvertWaveform);

    return 0;
}

int TestChartDurationAnalytics()
{
    Chart chart;
    chart.KeyAmount = 4;

    // Test on empty chart
    ASSERT(chart.GetFirstNoteTime() == -1);
    ASSERT(chart.GetLastNoteTime() == -1);
    ASSERT(chart.GetPlayableDuration() == 0);

    // Inject notes and hold
    chart.InjectNote(1500, 0, Note::EType::Common);
    chart.InjectHold(2000, 4500, 1);
    chart.InjectNote(3000, 2, Note::EType::Common);

    ASSERT(chart.GetFirstNoteTime() == 1500);
    ASSERT(chart.GetLastNoteTime() == 4500); // Hold tail ends at 4500
    ASSERT(chart.GetPlayableDuration() == 3000); // 4500 - 1500

    return 0;
}

int TestColumnNoteDistribution()
{
    Chart chart;
    chart.KeyAmount = 4;

    // Inject notes across specific columns
    chart.InjectNote(1000, 0, Note::EType::Common);
    chart.InjectNote(1500, 0, Note::EType::Common);
    chart.InjectNote(2000, 1, Note::EType::Common);
    chart.InjectNote(2500, 3, Note::EType::Common);
    chart.InjectNote(3000, 3, Note::EType::Common);
    chart.InjectNote(3500, 3, Note::EType::Common);

    std::vector<int> dist = chart.GetColumnNoteDistribution();
    ASSERT(dist.size() == 4);
    ASSERT(dist[0] == 2);
    ASSERT(dist[1] == 1);
    ASSERT(dist[2] == 0); // column 2 has 0 notes
    ASSERT(dist[3] == 3);

    return 0;
}

int TestChartClearNotesByType()
{
    Chart chart;
    chart.KeyAmount = 4;

    chart.InjectNote(1000, 0, Note::EType::Common);
    chart.InjectNote(1500, 1, Note::EType::Mine);
    chart.InjectNote(2000, 2, Note::EType::Mine);
    chart.InjectNote(2500, 3, Note::EType::Fake);

    // Clear only Mine notes
    int cleared = chart.ClearNotesByType(Note::EType::Mine);
    ASSERT(cleared == 2);

    // Verify common and fake notes remain intact
    ASSERT(chart.FindNote(1000, 0) != nullptr);
    ASSERT(chart.FindNote(2500, 3) != nullptr);
    ASSERT(chart.FindNote(1500, 1) == nullptr);
    ASSERT(chart.FindNote(2000, 2) == nullptr);

    return 0;
}

int TestWaveformResolutionMultiplier()
{
    WaveFormModule waveform;
    assert(waveform.GetResolutionMultiplier() == 1.0f);

    waveform.SetResolutionMultiplier(2.5f);
    assert(waveform.GetResolutionMultiplier() == 2.5f);

    // Test bounds clamping [0.25, 4.0]
    waveform.SetResolutionMultiplier(10.0f);
    assert(waveform.GetResolutionMultiplier() == 4.0f);

    waveform.SetResolutionMultiplier(0.01f);
    assert(waveform.GetResolutionMultiplier() == 0.25f);

    Configuration config;
    assert(config.WaveformResolutionMultiplier == 1.0f);
    config.WaveformResolutionMultiplier = 2.0f;
    assert(config.WaveformResolutionMultiplier == 2.0f);

    return 0;
}

int TestChartNoteStatistics()
{
    Chart chart;
    chart.KeyAmount = 4;

    // Inject common notes, holds, rolls, mines, lifts, fakes
    chart.InjectNote(1000, 0, Note::EType::Common);
    chart.InjectNote(1500, 1, Note::EType::Common);
    chart.InjectHold(2000, 2500, 2);
    chart.InjectRoll(3000, 3500, 3);
    chart.InjectNote(4000, 0, Note::EType::Mine);
    chart.InjectNote(4500, 1, Note::EType::Lift);
    chart.InjectNote(5000, 2, Note::EType::Fake);

    ChartNoteStatistics stats = chart.GetNoteStatistics();
    ASSERT(stats.TotalNotes == 9); // 2 taps + 2 hold endpoints (HoldBegin+HoldEnd) + 2 roll endpoints (RollBegin+RollEnd) + 1 mine + 1 lift + 1 fake
    ASSERT(stats.TapNotes == 2);
    ASSERT(stats.HoldNotes == 1);
    ASSERT(stats.RollNotes == 1);
    ASSERT(stats.MineNotes == 1);
    ASSERT(stats.LiftNotes == 1);
    ASSERT(stats.FakeNotes == 1);

    return 0;
}

int TestWaveformAlphaConfiguration()
{
    WaveFormModule waveform;
    assert(waveform.GetAlpha() == 128);

    waveform.SetAlpha(200);
    assert(waveform.GetAlpha() == 200);

    Configuration config;
    assert(config.WaveformAlpha == 128);
    config.WaveformAlpha = 180;
    assert(config.WaveformAlpha == 180);

    return 0;
}

int TestMultiDifficultyScan()
{
    ChartParserModule parser;
    // Scanning a non-existent path should safely return an empty vector without crashing
    auto emptyList = parser.ScanForCharts("data/charts/non_existent.osu");
    ASSERT(emptyList.empty());

    // Check ChartDefinition structure field handling
    ChartDefinition def;
    def.DifficultyName = "Hard";
    def.Creator = "Charter";
    def.ChartType = "dance-single";
    ASSERT(def.DifficultyName == "Hard");
    ASSERT(def.Creator == "Charter");
    ASSERT(def.ChartType == "dance-single");

    return 0;
}

int TestUnsnappedNoteValidation()
{
    Chart chart;
    chart.KeyAmount = 4;
    chart.InjectBpmPoint(0, 120.0, 500.0);

    // Note with valid snap
    chart.InjectNote(1000, 0, Note::EType::Common, -1, -1, 4);
    ChartValidationResult resValid = chart.ValidateChart();
    ASSERT(resValid.WarningCount == 0);

    // Note with unsnapped / unassigned snap
    chart.InjectNote(1123, 1, Note::EType::Common, -1, -1, -1);
    ChartValidationResult resUnsnapped = chart.ValidateChart();
    ASSERT(resUnsnapped.WarningCount > 0);

    return 0;
}

int TestSVEditing()
{
    Chart chart;
    chart.InjectSV(1000, 2.0);

    // Test Move
    ScrollVelocityMultiplier* s = chart.GetSVsRelatedToTimeRange(0, 2000)[0];
    ScrollVelocityMultiplier initial = *s;
    chart.MoveSV(initial, 1500);

    auto svs = chart.GetSVsRelatedToTimeRange(0, 2000);
    ASSERT(svs.size() == 1);
    ASSERT(svs[0]->TimePoint == 1500);
    ASSERT(svs[0]->Multiplier == 2.0);

    // Test Revaluate (In-Place)
    ScrollVelocityMultiplier initial2 = *svs[0];
    svs[0]->TimePoint = 1800;

    chart.RevaluateSV(initial2, *svs[0]);
    svs = chart.GetSVsRelatedToTimeRange(0, 2000);
    ASSERT(svs[0]->TimePoint == 1800);

    // Test Remove
    chart.RemoveSV(*svs[0]);
    svs = chart.GetSVsRelatedToTimeRange(0, 2000);
    ASSERT(svs.empty());

    return 0;
}

int TestStreamGen() {
    Chart chart;
    chart.KeyAmount = 4;
    chart.InjectBpmPoint(0, 120.0, 500.0);

    // Generate Stream
    // Divisor 16 means 16th notes (4 notes per beat).
    // BeatLength = 500ms.
    // Step = 500 * (4/16) = 125ms.
    chart.GenerateStream(0, 500, 16, StreamPattern::Staircase);

    // 0, 125, 250, 375
    ASSERT(chart.FindNote(0, 0) != nullptr);
    ASSERT(chart.FindNote(125, 1) != nullptr);
    ASSERT(chart.FindNote(250, 2) != nullptr);
    ASSERT(chart.FindNote(375, 3) != nullptr);

    return 0;
}

int TestNewTypes() {
    Chart chart;
    chart.KeyAmount = 4;

    // Test Roll
    chart.InjectRoll(1000, 2000, 0);
    Note* rollHead = chart.FindNote(1000, 0);
    ASSERT(rollHead != nullptr);
    ASSERT(rollHead->Type == Note::EType::RollBegin);
    ASSERT(rollHead->TimePointEnd == 2000);

    Note* rollTail = chart.FindNote(2000, 0);
    ASSERT(rollTail != nullptr);
    ASSERT(rollTail->Type == Note::EType::RollEnd);

    // Test Mine
    chart.InjectNote(3000, 1, Note::EType::Mine);
    Note* mine = chart.FindNote(3000, 1);
    ASSERT(mine != nullptr);
    ASSERT(mine->Type == Note::EType::Mine);

    // Test Lift
    chart.InjectNote(4000, 2, Note::EType::Lift);
    Note* lift = chart.FindNote(4000, 2);
    ASSERT(lift != nullptr);
    ASSERT(lift->Type == Note::EType::Lift);

    // Test Fake
    chart.InjectNote(5000, 3, Note::EType::Fake);
    Note* fake = chart.FindNote(5000, 3);
    ASSERT(fake != nullptr);
    ASSERT(fake->Type == Note::EType::Fake);

    // Test Move Note with Roll
    // Move Roll 1000->2000 to 1500->2000 (Resize)
    chart.MoveNote(1000, 1500, 0, 0, -1);

    // Verify old removed
    ASSERT(chart.FindNote(1000, 0) == nullptr);

    // Verify new exists
    Note* newHead = chart.FindNote(1500, 0);
    ASSERT(newHead != nullptr);
    ASSERT(newHead->Type == Note::EType::RollBegin);
    ASSERT(newHead->TimePointEnd == 2000);

    Note* newTail = chart.FindNote(2000, 0);
    ASSERT(newTail != nullptr);
    ASSERT(newTail->Type == Note::EType::RollEnd);

    return 0;
}

int TestStops()
{
    Chart chart;
    chart.InjectStop(1000, 2.5); // 2.5 seconds
    chart.InjectStop(2000, 1.0);

    int count = 0;
    chart.IterateAllStops([&](StopPoint& s){
        if (s.TimePoint == 1000 && s.Length == 2.5) count++;
        if (s.TimePoint == 2000 && s.Length == 1.0) count++;
    });

    ASSERT(count == 2);
    return 0;
}

int TestStopEditing()
{
    Chart chart;
    chart.InjectStop(1000, 2.0);

    // Test Move
    StopPoint* s = chart.GetStopsRelatedToTimeRange(0, 2000)[0];
    StopPoint initial = *s;
    chart.MoveStop(initial, 1500); // Move to 1500

    auto stops = chart.GetStopsRelatedToTimeRange(0, 2000);
    ASSERT(stops.size() == 1);
    ASSERT(stops[0]->TimePoint == 1500);
    ASSERT(stops[0]->Length == 2.0);

    // Test Revaluate (In-Place)
    StopPoint initial2 = *stops[0];
    stops[0]->TimePoint = 1800;

    chart.RevaluateStop(initial2, *stops[0]);
    stops = chart.GetStopsRelatedToTimeRange(0, 2000);
    ASSERT(stops[0]->TimePoint == 1800);

    // Test Remove
    chart.RemoveStop(*stops[0]);
    stops = chart.GetStopsRelatedToTimeRange(0, 2000);
    ASSERT(stops.empty());

    return 0;
}

int TestChartValidation()
{
    Chart chart;
    chart.KeyAmount = 4;

    // Normal notes
    chart.InjectNote(1000, 0, Note::EType::Common);
    chart.InjectHold(2000, 2500, 1);

    ChartValidationResult res = chart.ValidateChart();
    ASSERT(res.IsValid);
    ASSERT(res.ErrorCount == 0);

    // Inject invalid hold (End <= Begin)
    chart.InjectHold(3000, 3000, 2);
    ChartValidationResult resInvalid = chart.ValidateChart();
    ASSERT(!resInvalid.IsValid);
    ASSERT(resInvalid.ErrorCount > 0);

    return 0;
}

int TestShortcutMappings()
{
    ShortcutMenuModule shortcuts;
    shortcuts.RegisterShortcut("Undo", sf::Keyboard::Key::LControl, sf::Keyboard::Key::Z);
    shortcuts.RegisterShortcut("Redo", sf::Keyboard::Key::LControl, sf::Keyboard::Key::Y);

    ASSERT(shortcuts.GetShortcutString("Undo") == "CTRL+Z");
    ASSERT(shortcuts.GetShortcutString("Redo") == "CTRL+Y");

    shortcuts.UpdateShortcut("Undo", sf::Keyboard::Key::LControl, sf::Keyboard::Key::U);
    ASSERT(shortcuts.GetShortcutString("Undo") == "CTRL+U");

    return 0;
}

int TestSkinSnapColorTable()
{
    Skin skin;
    skin.LoadResources(4, "assets/data/skins/default");

    sf::Color red = skin.GetSnapColor(1);
    ASSERT(red == sf::Color(255, 0, 0, 255));

    sf::Color blue = skin.GetSnapColor(2);
    ASSERT(blue == sf::Color(0, 0, 255, 255));

    sf::Color yellow = skin.GetSnapColor(4);
    ASSERT(yellow == sf::Color(255, 255, 0, 255));

    ASSERT(skin.GetLoadedKeyAmount() == 4);

    return 0;
}

int TestWaveformBounds()
{
    WaveFormModule waveform;
    ASSERT(!waveform.HasWaveFormData());
    ASSERT(waveform.GetSongLength() == 0);

    WaveFormData dummyData[10];
    for (int i = 0; i < 10; ++i)
    {
        dummyData[i].Left = 0.5f;
        dummyData[i].Right = 0.5f;
    }

    waveform.SetWaveFormData(dummyData, 10);
    ASSERT(waveform.HasWaveFormData());
    ASSERT(waveform.GetSongLength() == 10);

    return 0;
}

int TestAudioFFTData()
{
    AudioModule audio;
    float fftBuffer[1024] = { 0 };
    int samplesRead = audio.GetFFTSpectrum(fftBuffer, 1024);
    // Without active loaded stream, should safely return 0 without crashing
    ASSERT(samplesRead == 0);
    return 0;
}

int TestWaveformColorConfiguration()
{
    WaveFormModule waveform;
    ASSERT(waveform.GetFrontColor().r == 0);
    ASSERT(waveform.GetFrontColor().g == 255);
    ASSERT(waveform.GetFrontColor().b == 255);

    waveform.SetFrontColor(sf::Color(255, 128, 64, 128));
    ASSERT(waveform.GetFrontColor().r == 255);
    ASSERT(waveform.GetFrontColor().g == 128);
    ASSERT(waveform.GetFrontColor().b == 64);

    Configuration config;
    ASSERT(config.WaveformColorRed == 0);
    ASSERT(config.WaveformColorGreen == 255);
    ASSERT(config.WaveformColorBlue == 255);

    config.WaveformColorRed = 200;
    config.WaveformColorGreen = 100;
    config.WaveformColorBlue = 50;
    ASSERT(config.WaveformColorRed == 200);
    ASSERT(config.WaveformColorGreen == 100);
    ASSERT(config.WaveformColorBlue == 50);

    return 0;
}

int main() {
    int result = 0;
    TEST(TestChartLogic);
    TEST(TestStreamGen);
    TEST(TestNewTypes);
    TEST(TestStops);
    TEST(TestStopEditing);
    TEST(TestSVEditing);
    TEST(TestChartValidation);
    TEST(TestShortcutMappings);
    TEST(TestUnsnappedNoteValidation);
    TEST(TestSkinSnapColorTable);
    TEST(TestWaveformBounds);
    TEST(TestAudioFFTData);
    TEST(TestWaveformColorConfiguration);
    TEST(TestMultiDifficultyScan);
    TEST(TestWaveformAlphaConfiguration);
    TEST(TestChartNoteStatistics);
    TEST(TestWaveformResolutionMultiplier);
    TEST(TestChartClearNotesByType);
    TEST(TestColumnNoteDistribution);
    TEST(TestChartDurationAnalytics);
    TEST(TestWaveformInversion);
    TEST(TestChartShiftNotesByColumn);
    TEST(TestWaveformWidthScale);
    TEST(TestChartClampNotesToTimeRange);
    TEST(TestNoteCountForColumn);
    TEST(TestWaveformCenterOffset);

    if (result == 0) std::cout << "All tests passed!" << std::endl;
    return result;
}
