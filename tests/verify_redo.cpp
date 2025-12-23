#include <iostream>
#include <cassert>
#include "../source/structures/chart.h"
#include "../source/modules/chart-parser-module.h"

// Define test framework
#define ASSERT(cond) if(!(cond)) { std::cerr << "Assertion failed: " << #cond << std::endl; return 1; }
#define TEST(name) std::cout << "Running " << #name << "..." << std::endl; if(name() != 0) { std::cerr << #name << " FAILED" << std::endl; return 1; } else { std::cout << #name << " PASSED" << std::endl; }

int TestRedo() {
    Chart chart;
    chart.KeyAmount = 4;

    // State 0: 1000
    chart.InjectNote(1000, 0, Note::EType::Common);

    // State 1: 1000, 2000
    chart.RegisterTimeSliceHistory(2000);
    chart.InjectNote(2000, 1, Note::EType::Common, -1, -1, -1, true); // true = skip redundant OnModified for test

    // State 2: 1000, 2000, 3000
    chart.RegisterTimeSliceHistory(3000);
    chart.InjectNote(3000, 2, Note::EType::Common, -1, -1, -1, true);

    ASSERT(chart.FindNote(3000, 2) != nullptr);

    // Undo -> State 1
    ASSERT(chart.Undo() == true);
    // Note: Undo restores the TimeSlice at 3000. Before inject, it was empty.
    // So FindNote(3000, 2) should fail if we restored the empty slice.
    // BUT: InjectNote modifies the slice. Does RegisterTimeSliceHistory save the state BEFORE modification?
    // Usage in Chart::PlaceNote:
    // RegisterTimeSliceHistory(InTime);
    // InjectNote(...);
    //
    // RegisterTimeSliceHistory(InTime) -> Pushes CURRENT state of slice at InTime (which is empty/old) to history.
    // InjectNote -> Modifies slice.
    // Undo -> Pops history (empty/old state) -> Applies it.
    // So 3000 should be gone.

    ASSERT(chart.FindNote(3000, 2) == nullptr);
    ASSERT(chart.FindNote(2000, 1) != nullptr);

    // Undo -> State 0
    ASSERT(chart.Undo() == true);
    ASSERT(chart.FindNote(2000, 1) == nullptr);
    ASSERT(chart.FindNote(1000, 0) != nullptr);

    // Redo -> State 1
    ASSERT(chart.Redo() == true);
    ASSERT(chart.FindNote(2000, 1) != nullptr);
    ASSERT(chart.FindNote(3000, 2) == nullptr);

    // Redo -> State 2
    ASSERT(chart.Redo() == true);
    ASSERT(chart.FindNote(3000, 2) != nullptr);

    // Redo -> Fail (No more future)
    ASSERT(chart.Redo() == false);

    // Undo -> State 1
    chart.Undo();
    ASSERT(chart.FindNote(3000, 2) == nullptr);

    // New Action -> State 1' (Invalidates State 2)
    chart.RegisterTimeSliceHistory(4000);
    chart.InjectNote(4000, 3, Note::EType::Common);

    // Redo -> Should Fail
    ASSERT(chart.Redo() == false);

    return 0;
}

int main() {
    int result = 0;
    result |= TestRedo();

    if (result == 0) std::cout << "Redo Verification Passed!" << std::endl;
    return result;
}
