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

int TestStreamGen() {
    Chart chart;
    chart.KeyAmount = 4;
    chart.InjectBpmPoint(0, 120.0, 500.0);

    // Generate Stream
    chart.GenerateStream(0, 500, 4, StreamPattern::Staircase);

    // 0, 125, 250, 375
    ASSERT(chart.FindNote(0, 0) != nullptr);
    ASSERT(chart.FindNote(125, 1) != nullptr);
    ASSERT(chart.FindNote(250, 2) != nullptr);
    ASSERT(chart.FindNote(375, 3) != nullptr);

    return 0;
}

int main() {
    int result = 0;
    result |= TestChartLogic();
    result |= TestStreamGen();

    if (result == 0) std::cout << "All tests passed!" << std::endl;
    return result;
}
