#include <iostream>
#include <cassert>
#include <vector>
#include <cmath>

#include "../source/structures/chart.h"
#include "../source/modules/chart-parser-module.h"

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

int main() {
    int result = 0;
    TEST(TestChartLogic);
    TEST(TestStreamGen);
    TEST(TestNewTypes);
    TEST(TestStops);
    TEST(TestStopEditing);
    TEST(TestSVEditing);

    if (result == 0) std::cout << "All tests passed!" << std::endl;
    return result;
}
