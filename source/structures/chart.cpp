#include "chart.h"

#include <iostream>
#include <algorithm>
#include <set>
#include <unordered_set>
#include <limits>
#include <random>

void NoteReferenceCollection::PushNote(Column InColumn, Note* InNote)
{
	HasNotes = true;
	NoteAmount++;

	if(ColumnNoteCount.find(InColumn) == ColumnNoteCount.end())
		ColumnNoteCount[InColumn] = 0;

	ColumnNoteCount[InColumn] += 1;
	Notes[InColumn].insert(InNote);

	HighestColumnAmount = std::max(HighestColumnAmount, ColumnNoteCount[InColumn]);

	switch (InNote->Type)
	{
	case Note::EType::Common:
		TrySetMinMaxTime(InNote->TimePoint);
		break;

	case Note::EType::HoldBegin:
		TrySetMinMaxTime(InNote->TimePointEnd);
		break;
	}
}

void NoteReferenceCollection::Clear()
{
	HasNotes = false;
	NoteAmount = 0;
	HighestColumnAmount = 0;

	Notes.clear();
	ColumnNoteCount.clear();

	MinTimePoint = std::numeric_limits<int>::max();
	MaxTimePoint = std::numeric_limits<int>::min();
}

void NoteReferenceCollection::TrySetMinMaxTime(Time InTime)
{
	MinTimePoint = std::min(MinTimePoint, InTime);
	MaxTimePoint = std::max(MaxTimePoint, InTime);
}

bool Chart::PlaceNote(const Time InTime, const Column InColumn, const int InBeatSnap)
{
	if (IsAPotentialNoteDuplicate(InTime, InColumn))
		return false;

	RegisterTimeSliceHistory(InTime);
	InjectNote(InTime, InColumn, Note::EType::Common, -1, -1, InBeatSnap);

	return true;
}

bool Chart::PlaceHold(const Time InTimeBegin, const Time InTimeEnd, const Column InColumn, const int InBeatSnap, const int InBeatSnapEnd)
{
	if (InTimeBegin == InTimeEnd)
		return PlaceNote(InTimeBegin, InColumn, InBeatSnap);

	if (IsAPotentialNoteDuplicate(InTimeBegin, InColumn) || IsAPotentialNoteDuplicate(InTimeEnd, InColumn))
		return false;

	RegisterTimeSliceHistoryRanged(InTimeBegin, InTimeEnd);

	InjectHold(InTimeBegin, InTimeEnd, InColumn, InBeatSnap);

	return true;
}

bool Chart::PlaceBpmPoint(const Time InTime, const double InBpm, const double InBeatLength)
{
	RegisterTimeSliceHistory(InTime);
	InjectBpmPoint(InTime, InBpm, InBeatLength);

	return true;
}

void Chart::BulkPlaceNotes(const std::vector<std::pair<Column, Note>> &InNotes, const bool InSkipHistoryRegistering, const bool InSkipOnModified)
{
	Time timePointMin = InNotes.front().second.TimePoint;
	Time timePointMax = InNotes.back().second.TimePoint;

	// - TIMESLICE_LENGTH and + TIMESLICE_LENGTH accounts for potential resnaps (AAAAAA)
	if(!InSkipHistoryRegistering)
		RegisterTimeSliceHistoryRanged(timePointMin - TIMESLICE_LENGTH, timePointMax + TIMESLICE_LENGTH);

	for (const auto &[column, note] : InNotes)
	{
		switch (note.Type)
		{
		case Note::EType::Common:
			InjectNote(note.TimePoint, column, note.Type, -1, -1, -1, InSkipOnModified);
			break;

		case Note::EType::HoldBegin:
			InjectHold(note.TimePointBegin, note.TimePointEnd, column, -1, -1, InSkipOnModified);
			break;
		}
	}
}

void Chart::MirrorNotes(NoteReferenceCollection& OutNotes)
{
	std::vector<std::pair<Column, Note>> bulkOfNotes;

	RegisterTimeSliceHistoryRanged(OutNotes.MinTimePoint, OutNotes.MaxTimePoint);

	for (auto& [column, notes] : OutNotes.Notes)
	{
		Column newColumn = (KeyAmount - 1) - column;

		//this is neccesary since removing an element form a vector will change the contents of a pointer pointing to that element
		std::vector<Note> copiedNotes;
		for (auto &note : notes)
			copiedNotes.push_back(*note);

		for (auto &note : copiedNotes)
		{
			bulkOfNotes.push_back({newColumn, note});
			RemoveNote(note.TimePoint, column, false, true, true);
		}
	}

	BulkPlaceNotes(bulkOfNotes, true, true);

	IterateTimeSlicesInTimeRange(OutNotes.MinTimePoint, OutNotes.MaxTimePoint, [this](TimeSlice& InTimeSlice)
	{
		_OnModified(InTimeSlice);
	});

	OutNotes.Clear();
}

void Chart::MirrorNotes(std::vector<std::pair<Column, Note>>& OutNotes)
{
	for(auto& [column, notes] : OutNotes)
		column = (KeyAmount - 1) - column;
}

void Chart::ScaleNotes(NoteReferenceCollection& OutNotes, float Factor)
{
	if (!OutNotes.HasNotes)
		return;

	// Use the minimum time in the selection as the pivot point
	Time pivotTime = OutNotes.MinTimePoint;

	// Prepare new notes list
	std::vector<std::pair<Column, Note>> scaledNotes;

	// Also we need to register history for the range we are modifying.
	// We are modifying the range [MinTime, MaxTime] -> [MinTime, MinTime + (Max-Min)*Factor]
	Time scaledMaxTime = pivotTime + (Time)((OutNotes.MaxTimePoint - pivotTime) * Factor);
	RegisterTimeSliceHistoryRanged(pivotTime, std::max(OutNotes.MaxTimePoint, scaledMaxTime) + TIMESLICE_LENGTH);

	for (auto& [column, notes] : OutNotes.Notes)
	{
		// Need to copy notes because we are going to remove them from the chart
		std::vector<Note> copiedNotes;
		for (auto& notePtr : notes)
			copiedNotes.push_back(*notePtr);

		for (auto& note : copiedNotes)
		{
			// Remove the old note
			RemoveNote(note.TimePoint, column, false, true, true);

			// Calculate new time
			Time newTime = pivotTime + (Time)((note.TimePoint - pivotTime) * Factor);
			note.TimePoint = newTime;

			// Reset beat snap as it likely changes (unless factor is integer, but safer to reset)
			note.BeatSnap = -1;

			if (note.Type == Note::EType::HoldBegin)
			{
				Time newEnd = pivotTime + (Time)((note.TimePointEnd - pivotTime) * Factor);
				note.TimePointBegin = newTime;
				note.TimePointEnd = newEnd;
			}

			scaledNotes.push_back({column, note});
		}
	}

	// Place new notes
	BulkPlaceNotes(scaledNotes, true, true);

	// Notify modification
	IterateTimeSlicesInTimeRange(pivotTime, std::max(OutNotes.MaxTimePoint, scaledMaxTime) + TIMESLICE_LENGTH, [this](TimeSlice& InTimeSlice)
	{
		_OnModified(InTimeSlice);
	});

	// Clear selection as pointers are invalid now
	OutNotes.Clear();
}

void Chart::ReverseNotes(NoteReferenceCollection& OutNotes)
{
	if (!OutNotes.HasNotes)
		return;

	Time minTime = OutNotes.MinTimePoint;
	Time maxTime = OutNotes.MaxTimePoint;

	RegisterTimeSliceHistoryRanged(minTime, maxTime + TIMESLICE_LENGTH);

	std::vector<std::pair<Column, Note>> reversedNotes;

	for (auto& [column, notes] : OutNotes.Notes)
	{
		std::vector<Note> copiedNotes;
		for (auto& notePtr : notes)
			copiedNotes.push_back(*notePtr);

		for (auto& note : copiedNotes)
		{
			RemoveNote(note.TimePoint, column, false, true, true);

			// Logic: newTime = max + min - oldTime
			Time newTime = maxTime + minTime - note.TimePoint;

			note.TimePoint = newTime;
			note.BeatSnap = -1;

			if (note.Type == Note::EType::HoldBegin)
			{
				// Hold end also flips
				Time newStart = maxTime + minTime - note.TimePointEnd;
				// note.TimePoint (newTime) is actually the new End of the reversed hold
				// Wait, if note.TimePoint is Start:
				// Old: Start -> End.
				// New Start = Max + Min - End.
				// New End   = Max + Min - Start.

				note.TimePointEnd = newTime;
				note.TimePointBegin = newStart;

				// Fix main TimePoint to be the Begin
				note.TimePoint = newStart;
			}

			reversedNotes.push_back({column, note});
		}
	}

	BulkPlaceNotes(reversedNotes, true, true);

	IterateTimeSlicesInTimeRange(minTime, maxTime + TIMESLICE_LENGTH, [this](TimeSlice& InTimeSlice)
	{
		_OnModified(InTimeSlice);
	});

	OutNotes.Clear();
}

void Chart::ConvertToHolds(NoteReferenceCollection& OutNotes, Time Length)
{
    if (!OutNotes.HasNotes || Length <= 0) return;

    RegisterTimeSliceHistoryRanged(OutNotes.MinTimePoint, OutNotes.MaxTimePoint + Length + TIMESLICE_LENGTH);

    std::vector<std::pair<Column, Note>> newNotes;

    for (auto& [column, notes] : OutNotes.Notes)
    {
        std::vector<Note> copiedNotes;
        for (auto& notePtr : notes)
            copiedNotes.push_back(*notePtr);

        for (auto& note : copiedNotes)
        {
            RemoveNote(note.TimePoint, column, false, true, true);

            note.Type = Note::EType::HoldBegin;
            note.TimePointBegin = note.TimePoint;
            // If already hold, extend? Or reset?
            // "Convert to Hold" usually means set length.
            note.TimePointEnd = note.TimePoint + Length;

            newNotes.push_back({column, note});
        }
    }

    BulkPlaceNotes(newNotes, true, true);

    IterateTimeSlicesInTimeRange(OutNotes.MinTimePoint, OutNotes.MaxTimePoint + Length + TIMESLICE_LENGTH, [this](TimeSlice& InTimeSlice)
    {
        _OnModified(InTimeSlice);
    });

    OutNotes.Clear();
}

void Chart::ConvertToTaps(NoteReferenceCollection& OutNotes)
{
    if (!OutNotes.HasNotes) return;

    RegisterTimeSliceHistoryRanged(OutNotes.MinTimePoint, OutNotes.MaxTimePoint + TIMESLICE_LENGTH);

    std::vector<std::pair<Column, Note>> newNotes;

    for (auto& [column, notes] : OutNotes.Notes)
    {
        std::vector<Note> copiedNotes;
        for (auto& notePtr : notes)
            copiedNotes.push_back(*notePtr);

        for (auto& note : copiedNotes)
        {
            RemoveNote(note.TimePoint, column, false, true, true);

            note.Type = Note::EType::Common;
            note.TimePointBegin = 0;
            note.TimePointEnd = 0;

            newNotes.push_back({column, note});
        }
    }

    BulkPlaceNotes(newNotes, true, true);

    IterateTimeSlicesInTimeRange(OutNotes.MinTimePoint, OutNotes.MaxTimePoint + TIMESLICE_LENGTH, [this](TimeSlice& InTimeSlice)
    {
        _OnModified(InTimeSlice);
    });

    OutNotes.Clear();
}

void Chart::GenerateStream(Time Start, Time End, int Divisor, StreamPattern Pattern)
{
	if (Start >= End || Divisor <= 0) return;

	RegisterTimeSliceHistoryRanged(Start - TIMESLICE_LENGTH, End + TIMESLICE_LENGTH);

	// Create beats
	// Find starting BPM point
	BpmPoint* bpm = GetPreviousBpmPointFromTimePoint(Start);
	if (!bpm) bpm = GetNextBpmPointFromTimePoint(-1000000);

	if (!bpm) return; // No timing info

	// Start placing notes
	Time currentTime = Start;
	int noteIndex = 0;

	// Setup randomness
	std::random_device rd;
	std::mt19937 g(rd());
	std::uniform_int_distribution<> distrib(0, KeyAmount - 1);

	int lastCol = -1;
	int lastLastCol = -1;

	while (currentTime < End)
	{
		Column col = 0;

		switch (Pattern)
		{
			case StreamPattern::Staircase:
				col = noteIndex % KeyAmount;
				break;

			case StreamPattern::Trill:
				if (KeyAmount >= 2) col = (noteIndex % 2) + (KeyAmount / 2 - 1);
				else col = 0;
				break;

			case StreamPattern::Spiral:
				col = noteIndex % KeyAmount;
				break;

			case StreamPattern::Random:
				do {
					col = distrib(g);
				} while (col == lastCol || (col == lastLastCol && KeyAmount > 2));
				break;

            case StreamPattern::Jumpstream:
            case StreamPattern::Handstream:
            case StreamPattern::Chordjack:
                // Special handling below
                break;
		}

        if (Pattern == StreamPattern::Jumpstream || Pattern == StreamPattern::Handstream || Pattern == StreamPattern::Chordjack)
        {
            int count = 2;
            if (Pattern == StreamPattern::Handstream) count = 3;
            if (Pattern == StreamPattern::Chordjack) count = std::min(4, KeyAmount); // Usually 3-4

            if (count > KeyAmount) count = KeyAmount;

            // Pick 'count' distinct columns
            std::vector<int> cols(KeyAmount);
            std::iota(cols.begin(), cols.end(), 0);
            std::shuffle(cols.begin(), cols.end(), g);

            for(int i=0; i<count; ++i)
            {
                Column c = cols[i];
                if (!FindNote(currentTime, c))
                    InjectNote(currentTime, c, Note::EType::Common);
            }
        }
        else
        {
            if (col < KeyAmount)
            {
                if (!FindNote(currentTime, col))
                    InjectNote(currentTime, col, Note::EType::Common);
            }
            lastLastCol = lastCol;
            lastCol = col;
        }
		noteIndex++;

		// Advance time based on current BPM
		// We need to re-check BPM at current time?
		// Yes, if BPM changes.
		BpmPoint* currentBpm = GetPreviousBpmPointFromTimePoint(currentTime);
		if (currentBpm)
		{
            // Divisor is Measure Divisor (4, 8, 16...). BeatLength is 1 beat (1/4 measure).
            // Step = BeatLength * (4.0 / Divisor)
			double step = currentBpm->BeatLength * (4.0 / double(Divisor));
			currentTime += Time(step);
		}
		else
		{
			break; // Should not happen if we started with one
		}
	}

	IterateTimeSlicesInTimeRange(Start - TIMESLICE_LENGTH, End + TIMESLICE_LENGTH, [this](TimeSlice& InTimeSlice)
	{
		_OnModified(InTimeSlice);
	});
}

std::vector<float> Chart::CalculateNPSGraph(int WindowSizeMs)
{
	if (!_BpmPointCounter || WindowSizeMs <= 0)
		return {};

	// Find song length (approximate from max time point of notes)
	Time maxTime = 0;
	IterateAllNotes([&maxTime](Note& n, Column c) {
		maxTime = std::max(maxTime, n.TimePoint);
	});

	int numWindows = (maxTime / WindowSizeMs) + 2;
	std::vector<int> counts(numWindows, 0);

	IterateAllNotes([&counts, WindowSizeMs](Note& n, Column c) {
		if (n.TimePoint >= 0)
		{
			int idx = n.TimePoint / WindowSizeMs;
			if (idx < counts.size())
				counts[idx]++;
		}
	});

	std::vector<float> nps(numWindows);
	for (size_t i = 0; i < counts.size(); ++i)
	{
		nps[i] = float(counts[i]) * (1000.0f / float(WindowSizeMs));
	}

	return nps;
}

float Chart::GetAverageNPS()
{
	// Total notes / Total drain time
	int noteCount = 0;
	Time firstNote = std::numeric_limits<int>::max();
	Time lastNote = std::numeric_limits<int>::min();

	IterateAllNotes([&](Note& n, Column c) {
		noteCount++;
		firstNote = std::min(firstNote, n.TimePoint);
		lastNote = std::max(lastNote, n.TimePoint);
	});

	if (noteCount == 0 || lastNote <= firstNote) return 0.0f;

	double seconds = double(lastNote - firstNote) / 1000.0;
	return float(noteCount) / float(seconds);
}

float Chart::GetPeakNPS()
{
	std::vector<float> graph = CalculateNPSGraph(1000); // 1s window
	float peak = 0.0f;
	for (float v : graph) peak = std::max(peak, v);
	return peak;
}

double Chart::GetBeatFromTime(Time InTime)
{
	if (!_BpmPointCounter)
		return 0.0;

	BpmPoint* bpmPoint = GetPreviousBpmPointFromTimePoint(InTime);

	if (!bpmPoint)
	{
		// Find first BPM point to approximate
		BpmPoint* first = GetNextBpmPointFromTimePoint(-1000000);
		if (first)
		{
			// Time < First BPM Point. Extrapolate backwards.
			// Beat = 0 - (FirstTime - Time) / (BeatLength)
			return - (double(first->TimePoint - InTime) / first->BeatLength);
		}
		return 0.0; // Should not happen if counter > 0
	}

	// We need the cumulative beat sum up to this BPM point.
	// This is expensive to calculate every time.
	// However, for Quantize, we just need the beat relative to the current BPM point to snap it?
	// No, to snap to a global grid (e.g. measure lines), we need absolute beats.
	// But usually Quantize snaps to the "local" grid defined by the current timing point.
	// StepMania snaps to the measure.

	// Let's implement a simple integration.
	double beat = 0.0;
	Time lastTime = 0; // Assuming 0 start? Or finding first BPM point.

	// Find all BPM points up to InTime
	std::vector<BpmPoint*> points;
	// We can't easily get all points in order without iterating everything.
	// Chart stores BPM points in TimeSlices.

	// Optimization: For "Quantize", we usually care about the fractional part relative to the current BPM measure.
	// If meter is 4/4, we align to 1/Divisor beats.
	// Current BPM point defines the grid anchor.

	// So:
	double timeDelta = double(InTime - bpmPoint->TimePoint);
	double beatsSinceBpm = timeDelta / bpmPoint->BeatLength;

	// We assume the BPM point itself is on a beat (usually beat 0 or start of a measure).
	// Ideally we should know the absolute beat of the BPM point.
	// But if we assume the user placed BPM points on beats, we can snap relative to them.

	return beatsSinceBpm;
}

Time Chart::GetTimeFromBeat(double InBeat)
{
	// Inverse of above, strictly local to current BPM point logic for now.
	// This might be insufficient for complex multi-BPM songs if we cross boundaries,
	// but Quantize usually works on individual notes.
	// If we quantize a note, we find its BPM point, snap the local beat, and convert back.

	// This function signature implies global beat, but we only have local logic easily available.
	// Let's rely on the caller to handle the context or make this "GetTimeFromLocalBeat(Beat, BpmPoint)".

	return 0; // Not used directly in this simplified logic
}

void Chart::QuantizeNotes(NoteReferenceCollection& OutNotes, int Divisor)
{
	if (!OutNotes.HasNotes || Divisor <= 0)
		return;

	RegisterTimeSliceHistoryRanged(OutNotes.MinTimePoint, OutNotes.MaxTimePoint + TIMESLICE_LENGTH);

	std::vector<std::pair<Column, Note>> quantizedNotes;

	for (auto& [column, notes] : OutNotes.Notes)
	{
		std::vector<Note> copiedNotes;
		for (auto& notePtr : notes)
			copiedNotes.push_back(*notePtr);

		for (auto& note : copiedNotes)
		{
			RemoveNote(note.TimePoint, column, false, true, true);

			// Find BPM point
			BpmPoint* bpm = GetPreviousBpmPointFromTimePoint(note.TimePoint);
			if (!bpm) bpm = GetNextBpmPointFromTimePoint(-1000000);

			if (bpm)
			{
				double beatLen = bpm->BeatLength;
				double timeDelta = double(note.TimePoint - bpm->TimePoint);
				double beat = timeDelta / beatLen;

				// Snap beat
				// Divisor is Measure Divisor (4 = Quarter Note = 1 Beat, 16 = 16th Note = 0.25 Beat)
                // Grid in beats = 4.0 / Divisor.
                // We want to snap 'beat' to multiples of 'grid'.
                double grid = 4.0 / double(Divisor);
				double snappedBeat = std::round(beat / grid) * grid;

				Time newTime = bpm->TimePoint + Time(snappedBeat * beatLen);
				note.TimePoint = newTime;

				// Calculate snap for metadata
				// 1/4 -> 1, 1/8 -> 2?
				// Note::BeatSnap is usually 1, 2, 3, 4, 6, 8, 12, 16...
				// Logic in BeatModule::GetBeatSnap.
				// We can try to set it or leave -1 (auto).
				// We will leave -1 or try to derive it.
				note.BeatSnap = -1;

				if (note.Type == Note::EType::HoldBegin)
				{
					// Quantize end too? Yes usually.
					// Or maintain length? ArrowVortex "Quantize" snaps both ends.

					// Recalculate for end
					BpmPoint* bpmEnd = GetPreviousBpmPointFromTimePoint(note.TimePointEnd);
					if (!bpmEnd) bpmEnd = GetNextBpmPointFromTimePoint(-1000000);

					if (bpmEnd)
					{
						double beatLenEnd = bpmEnd->BeatLength;
						double timeDeltaEnd = double(note.TimePointEnd - bpmEnd->TimePoint);
						double beatEnd = timeDeltaEnd / beatLenEnd;
                        double gridEnd = 4.0 / double(Divisor);
						double snappedBeatEnd = std::round(beatEnd / gridEnd) * gridEnd;
						note.TimePointEnd = bpmEnd->TimePoint + Time(snappedBeatEnd * beatLenEnd);
					}

					// Safety: End > Start
					if (note.TimePointEnd <= note.TimePoint)
						note.TimePointEnd = note.TimePoint + Time(bpm->BeatLength * (4.0 / Divisor)); // min length
				}
			}

			quantizedNotes.push_back({column, note});
		}
	}

	BulkPlaceNotes(quantizedNotes, true, true);

	IterateTimeSlicesInTimeRange(OutNotes.MinTimePoint, OutNotes.MaxTimePoint + TIMESLICE_LENGTH, [this](TimeSlice& InTimeSlice)
	{
		_OnModified(InTimeSlice);
	});

	OutNotes.Clear();
}

void Chart::ShuffleNotes(NoteReferenceCollection& OutNotes)
{
	if (!OutNotes.HasNotes)
		return;

	RegisterTimeSliceHistoryRanged(OutNotes.MinTimePoint, OutNotes.MaxTimePoint + TIMESLICE_LENGTH);

	// Generate permutation
	std::vector<int> p(KeyAmount);
	std::iota(p.begin(), p.end(), 0);

	std::random_device rd;
	std::mt19937 g(rd());
	std::shuffle(p.begin(), p.end(), g);

	std::vector<std::pair<Column, Note>> shuffledNotes;

	for (auto& [column, notes] : OutNotes.Notes)
	{
		Column newColumn = p[column];

		std::vector<Note> copiedNotes;
		for (auto& notePtr : notes)
			copiedNotes.push_back(*notePtr);

		for (auto& note : copiedNotes)
		{
			RemoveNote(note.TimePoint, column, false, true, true);
			shuffledNotes.push_back({newColumn, note});
		}
	}

	BulkPlaceNotes(shuffledNotes, true, true);

	IterateTimeSlicesInTimeRange(OutNotes.MinTimePoint, OutNotes.MaxTimePoint + TIMESLICE_LENGTH, [this](TimeSlice& InTimeSlice)
	{
		_OnModified(InTimeSlice);
	});

	OutNotes.Clear();
}

bool Chart::RemoveNote(const Time InTime, const Column InColumn, const bool InIgnoreHoldChecks, const bool InSkipHistoryRegistering, const bool InSkipOnModified)
{
	auto &timeSlice = FindOrAddTimeSlice(InTime);
	auto &noteCollection = timeSlice.Notes[InColumn];

	auto noteIt = std::find_if(noteCollection.begin(), noteCollection.end(), [InTime](const Note &InNote)
							   { return InNote.TimePoint == InTime; });

	if (noteIt == noteCollection.end())
		return false;

	//hold checks
	if (InIgnoreHoldChecks == false && (noteIt->Type == Note::EType::HoldBegin || noteIt->Type == Note::EType::HoldEnd))
	{
		Time holdTimeBegin = noteIt->TimePointBegin;
		Time holdTimedEnd = noteIt->TimePointEnd;

		if (!InSkipHistoryRegistering)
			RegisterTimeSliceHistoryRanged(holdTimeBegin, holdTimedEnd);

		//removes all intermidiate notes
		for (Time time = FindOrAddTimeSlice(holdTimeBegin).TimePoint + TIMESLICE_LENGTH;
			 time <= FindOrAddTimeSlice(holdTimedEnd).TimePoint - TIMESLICE_LENGTH;
			 time += TIMESLICE_LENGTH)
		{
			RemoveNote(time, InColumn, true);
		}

		RemoveNote(holdTimeBegin, InColumn, true);
		RemoveNote(holdTimedEnd, InColumn, true);
	}
	else
	{
		if (!InIgnoreHoldChecks && !InSkipHistoryRegistering)
			RegisterTimeSliceHistory(InTime);

		noteCollection.erase(noteIt);
	}

	if(!InSkipOnModified)
		_OnModified(timeSlice);

	return true;
}

bool Chart::RemoveBpmPoint(BpmPoint &InBpmPoint, const bool InSkipHistoryRegistering)
{
	auto &timeSlice = FindOrAddTimeSlice(InBpmPoint.TimePoint);
	auto &bpmCollection = timeSlice.BpmPoints;

	if (!InSkipHistoryRegistering)
		RegisterTimeSliceHistory(InBpmPoint.TimePoint);

	bpmCollection.erase(std::remove(bpmCollection.begin(), bpmCollection.end(), InBpmPoint), bpmCollection.end());

	CachedBpmPoints.clear();

	_BpmPointCounter--;

	return true;
}

bool Chart::BulkRemoveNotes(NoteReferenceCollection& InNotes, const bool InSkipHistoryRegistering)
{
	if (!InSkipHistoryRegistering)
		RegisterTimeSliceHistoryRanged(InNotes.MinTimePoint, InNotes.MaxTimePoint);

	for (auto& [column, notes] : InNotes.Notes)
	{
		Column newColumn = (KeyAmount - 1) - column;

		//this is neccesary since removing an element form a vector will change the contents of a pointer pointing to that element
		std::vector<Note> copiedNotes;
		for (auto &note : notes)
			copiedNotes.push_back(*note);

		for (auto &note : copiedNotes)
			RemoveNote(note.TimePoint, column, false, true, true);
	}

	IterateTimeSlicesInTimeRange(InNotes.MinTimePoint, InNotes.MaxTimePoint, [this](TimeSlice& InTimeSlice)
	{
		_OnModified(InTimeSlice);
	});

	InNotes.Clear();

	return true;
}

Note &Chart::InjectNote(const Time InTime, const Column InColumn, const Note::EType InNoteType, const Time InTimeBegin, const Time InTimeEnd, const int InBeatSnap, const bool InSkipOnModified)
{
	auto &timeSlice = FindOrAddTimeSlice(InTime);

	Note note;
	note.Type = InNoteType;
	note.TimePoint = InTime;
	note.BeatSnap = InBeatSnap;

	note.TimePointBegin = InTimeBegin;
	note.TimePointEnd = InTimeEnd;

	timeSlice.Notes[InColumn].push_back(note);
	Note &injectedNoteRef = timeSlice.Notes[InColumn].back();

	std::sort(timeSlice.Notes[InColumn].begin(), timeSlice.Notes[InColumn].end(), [](const auto &lhs, const auto &rhs)
			  { return (lhs.TimePoint < rhs.TimePoint); });

	if(!InSkipOnModified)
		_OnModified(timeSlice);

	return injectedNoteRef;
}

Note &Chart::InjectHold(const Time InTimeBegin, const Time InTimeEnd, const Column InColumn, const int InBeatSnapBegin, const int InBeatSnapEnd, const bool InSkipOnModified)
{
	Note &noteToReturn = InjectNote(InTimeBegin, InColumn, Note::EType::HoldBegin, InTimeBegin, InTimeEnd, InBeatSnapBegin);

	Time startTime = FindOrAddTimeSlice(InTimeBegin).TimePoint + TIMESLICE_LENGTH;
	Time endTime = FindOrAddTimeSlice(InTimeEnd).TimePoint - TIMESLICE_LENGTH;

	for (Time time = startTime; time <= endTime; time += TIMESLICE_LENGTH)
	{
		InjectNote(time, InColumn, Note::EType::HoldIntermediate, InTimeBegin, InTimeEnd);
	}

	if(!InSkipOnModified)
		InjectNote(InTimeEnd, InColumn, Note::EType::HoldEnd, InTimeBegin, InTimeEnd, InBeatSnapEnd);

	return noteToReturn;
}

BpmPoint *Chart::InjectBpmPoint(const Time InTime, const double InBpm, const double InBeatLength)
{
	auto &timeSlice = FindOrAddTimeSlice(InTime);

	BpmPoint bpmPoint;
	bpmPoint.TimePoint = InTime;
	bpmPoint.Bpm = InBpm;
	bpmPoint.BeatLength = InBeatLength;

	timeSlice.BpmPoints.push_back(bpmPoint);

	BpmPoint *bpmPointPtr = &(timeSlice.BpmPoints.back());

	std::sort(timeSlice.BpmPoints.begin(), timeSlice.BpmPoints.end(), [](const auto &lhs, const auto &rhs)
			  { return lhs.TimePoint < rhs.TimePoint; });

	_BpmPointCounter++;

	return bpmPointPtr;
}

Note *Chart::MoveNote(const Time InTimeFrom, const Time InTimeTo, const Column InColumnFrom, const Column InColumnTo, const int InNewBeatSnap)
{
	//have I mentioned that I really dislike handling edge-cases?
	Note noteToRemove = *FindNote(InTimeFrom, InColumnFrom);

	switch (noteToRemove.Type)
	{
	case Note::EType::Common:
	{
		auto &timeSliceFrom = FindOrAddTimeSlice(InTimeFrom);
		auto &timeSliceTo = FindOrAddTimeSlice(InTimeTo);

		if (timeSliceFrom.Index == timeSliceTo.Index)
			RegisterTimeSliceHistory(InTimeFrom);
		else
			RegisterTimeSliceHistoryRanged(InTimeFrom, InTimeTo);

		RemoveNote(InTimeFrom, InColumnFrom, false, true);
		return &(InjectNote(InTimeTo, InColumnTo, Note::EType::Common, -1, -1, InNewBeatSnap));
	}
	break;

	case Note::EType::HoldBegin:
	{
		if (InTimeTo < noteToRemove.TimePointBegin)
			RegisterTimeSliceHistoryRanged(InTimeTo - TIMESLICE_LENGTH, noteToRemove.TimePointEnd);
		else
			RegisterTimeSliceHistoryRanged(noteToRemove.TimePointBegin - TIMESLICE_LENGTH, noteToRemove.TimePointEnd);

		RemoveNote(InTimeFrom, InColumnFrom, false, true);

		return &(InjectHold(InTimeTo, noteToRemove.TimePointEnd, InColumnTo, InNewBeatSnap));
	}
	break;
	case Note::EType::HoldEnd:
	{
		if (InTimeTo > noteToRemove.TimePointBegin)
			RegisterTimeSliceHistoryRanged(noteToRemove.TimePointBegin, InTimeTo + TIMESLICE_LENGTH);
		else
			RegisterTimeSliceHistoryRanged(noteToRemove.TimePointBegin, noteToRemove.TimePointEnd + TIMESLICE_LENGTH);

		int beatSnap = FindNote(noteToRemove.TimePointBegin, InColumnFrom)->BeatSnap;

		RemoveNote(InTimeFrom, InColumnFrom, false, true);

		return &(InjectHold(noteToRemove.TimePointBegin, InTimeTo, InColumnTo, beatSnap));
	}
	break;

	default:
		return nullptr;
		break;
	}
}

Note *Chart::FindNote(const Time InTime, const Column InColumn)
{
	auto &timeSlice = FindOrAddTimeSlice(InTime);
	auto &noteCollection = timeSlice.Notes[InColumn];

	auto noteIt = std::find_if(noteCollection.begin(), noteCollection.end(), [InTime](const Note &InNote)
							   { return InNote.TimePoint == InTime; });

	if (noteIt == noteCollection.end())
		return nullptr;

	return &(*noteIt);
}

void Chart::DebugPrint()
{
	std::cout << DifficultyName << std::endl;
	std::cout << "***************************" << std::endl;

	for (auto [timePoint, slice] : TimeSlices)
	{
		for (auto [column, notes] : slice.Notes)
		{
			for (auto note : notes)
			{
				std::string type = "";
				switch (note.Type)
				{
				case Note::EType::Common:
					type = "common";
					break;
				case Note::EType::HoldBegin:
					type = "hold begin";
					break;
				case Note::EType::HoldIntermediate:
					type = "hold intermediate";
					break;
				case Note::EType::HoldEnd:
					type = "hold end";
					break;

				default:
					break;
				}

				std::cout << timePoint << ":" << std::to_string(note.TimePoint) << " - " << std::to_string(column) << " - " << type << std::endl;
			}
		}
	}

	std::cout << std::endl;
}

void Chart::RegisterOnModifiedCallback(std::function<void(TimeSlice &)> InCallback)
{
	_OnModified = InCallback;
}

bool Chart::IsAPotentialNoteDuplicate(const Time InTime, const Column InColumn)
{
	auto &notes = FindOrAddTimeSlice(InTime).Notes[InColumn];
	return (std::find_if(notes.begin(), notes.end(), [InTime](const Note &InNote)
						 { return InNote.TimePoint == InTime; }) != notes.end());
}

TimeSlice &Chart::FindOrAddTimeSlice(const Time InTime)
{
	int index = InTime / TIMESLICE_LENGTH;
	if (TimeSlices.find(index) == TimeSlices.end())
	{
		TimeSlices[index].TimePoint = index * TIMESLICE_LENGTH;
		TimeSlices[index].Index = index;
	}

	return TimeSlices[index];
}

void Chart::FillNoteCollectionWithAllNotes(NoteReferenceCollection& OutNotes)
{
	OutNotes.Clear();

	IterateAllNotes([&OutNotes](Note& InNote, Column InColumn)
	{
		OutNotes.PushNote(InColumn, &InNote);
	});
}

void Chart::PushTimeSliceHistoryIfNotAdded(const Time InTime)
{
	if (TimeSliceHistory.size() == 0)
		return;

	auto &collection = TimeSliceHistory.top();

	for (auto &timeSlice : collection)
	{
		if (InTime >= timeSlice.TimePoint && InTime <= timeSlice.TimePoint + TIMESLICE_LENGTH)
			return;
	}

	collection.push_back(FindOrAddTimeSlice(InTime));
}

void Chart::RevaluateBpmPoint(BpmPoint &InFormerBpmPoint, BpmPoint &InMovedBpmPoint)
{
	auto &formerTimeSlice = FindOrAddTimeSlice(InFormerBpmPoint.TimePoint);
	auto &newTimeSlice = FindOrAddTimeSlice(InMovedBpmPoint.TimePoint);

	auto &formerBpmCollection = formerTimeSlice.BpmPoints;

	if (formerTimeSlice.Index != newTimeSlice.Index)
	{
		BpmPoint bpmPointToAdd = InMovedBpmPoint;

		formerBpmCollection.erase(std::remove(formerBpmCollection.begin(), formerBpmCollection.end(), InMovedBpmPoint), formerBpmCollection.end());
		CachedBpmPoints.clear();

		TimeSliceHistory.top().push_back(newTimeSlice);

		InjectBpmPoint(bpmPointToAdd.TimePoint, bpmPointToAdd.Bpm, bpmPointToAdd.BeatLength);

		_BpmPointCounter--;
	}
}

void Chart::RegisterTimeSliceHistory(const Time InTime)
{
	TimeSliceHistory.push({FindOrAddTimeSlice(InTime)});
}

void Chart::RegisterTimeSliceHistoryRanged(const Time InTimeBegin, const Time InTimeEnd)
{
	std::vector<TimeSlice> timeSlices;

	IterateTimeSlicesInTimeRange(InTimeBegin, InTimeEnd, [&timeSlices](TimeSlice &InTimeSlice)
								 { timeSlices.push_back(InTimeSlice); });

	TimeSliceHistory.push(timeSlices);
}

bool Chart::Undo()
{
	if (TimeSliceHistory.empty())
		return false;

	for (auto &timeSlice : TimeSliceHistory.top())
		_OnModified(TimeSlices[timeSlice.Index] = timeSlice);

	TimeSliceHistory.pop();

	return true;
}

void Chart::IterateTimeSlicesInTimeRange(const Time InTimeBegin, const Time InTimeEnd, std::function<void(TimeSlice &)> InWork)
{
	if (InTimeBegin > InTimeEnd)
	{
		for (TimeSlice *timeSlice = &FindOrAddTimeSlice(InTimeBegin);
			 timeSlice->TimePoint > InTimeEnd - TIMESLICE_LENGTH;
			 timeSlice = &FindOrAddTimeSlice(timeSlice->TimePoint - TIMESLICE_LENGTH))
		{
			InWork(*timeSlice);
		}

		return;
	}

	for (TimeSlice *timeSlice = &FindOrAddTimeSlice(InTimeBegin);
		 timeSlice->TimePoint <= InTimeEnd;
		 timeSlice = &FindOrAddTimeSlice(timeSlice->TimePoint + TIMESLICE_LENGTH))
	{
		InWork(*timeSlice);
	}
}

void Chart::IterateNotesInTimeRange(const Time InTimeBegin, const Time InTimeEnd, std::function<void(Note &, const Column)> InWork)
{
	for (TimeSlice *timeSlice = &FindOrAddTimeSlice(InTimeBegin);
		 timeSlice->TimePoint <= InTimeEnd;
		 timeSlice = &FindOrAddTimeSlice(timeSlice->TimePoint + TIMESLICE_LENGTH))
	{
		for (auto &[column, notes] : timeSlice->Notes)
		{
			for (auto &note : notes)
			{
				if (note.TimePoint >= InTimeBegin && note.TimePoint <= InTimeEnd)
					InWork(note, column);
			}
		}
	}
}

void Chart::IterateAllNotes(std::function<void(Note &, const Column)> InWork)
{
	for (auto &[ID, timeSlice] : TimeSlices)
	{
		for (auto &[column, notes] : timeSlice.Notes)
		{
			for (auto &note : notes)
			{
				InWork(note, column);
			}
		}
	}
}

void Chart::IterateAllBpmPoints(std::function<void(BpmPoint &)> InWork)
{
	for (auto &[ID, timeSlice] : TimeSlices)
	{
		for (auto &bpmPoint : timeSlice.BpmPoints)
		{
			InWork(bpmPoint);
		}
	}
}

std::vector<BpmPoint *> &Chart::GetBpmPointsRelatedToTimeRange(const Time InTimeBegin, const Time InTimeEnd)
{
	CachedBpmPoints.clear();

	if (!_BpmPointCounter)
		return CachedBpmPoints;

	Time newTimeBegin = InTimeBegin - TIMESLICE_LENGTH;

	for (Time time = newTimeBegin; time >= -10000; time -= TIMESLICE_LENGTH)
	{
		auto &timeSlice = FindOrAddTimeSlice(time);
		if (!timeSlice.BpmPoints.empty())
		{
			newTimeBegin = time;
			break;
		}
	}

	IterateTimeSlicesInTimeRange(newTimeBegin, InTimeEnd, [this](TimeSlice &InTimeSlice)
								 {
									 if (InTimeSlice.BpmPoints.empty())
										 return;

									 for (auto &bpmPoint : InTimeSlice.BpmPoints)
										 CachedBpmPoints.push_back(&bpmPoint);
								 });

	if (CachedBpmPoints.empty())
		return GetBpmPointsRelatedToTimeRange(InTimeBegin, InTimeEnd + TIMESLICE_LENGTH);

	return CachedBpmPoints;
}

BpmPoint *Chart::GetPreviousBpmPointFromTimePoint(const Time InTime)
{
	BpmPoint *foundBpmPoint = nullptr;
	BpmPoint *previousBpmPoint = nullptr;

	IterateTimeSlicesInTimeRange(0, InTime, [this, InTime, &foundBpmPoint, &previousBpmPoint](TimeSlice &InTimeSlice)
								 {
									 if (InTimeSlice.BpmPoints.empty())
										 return;

									 for (auto &bpmPoint : InTimeSlice.BpmPoints)
										 if (bpmPoint.TimePoint <= InTime)
											 previousBpmPoint = &bpmPoint;
								 });

	foundBpmPoint = previousBpmPoint;

	return foundBpmPoint;
}

BpmPoint *Chart::GetNextBpmPointFromTimePoint(const Time InTime)
{
	if (!_BpmPointCounter)
		return nullptr;

	auto &relevantTimeSlice = FindOrAddTimeSlice(InTime);

	auto timeSliceIt = TimeSlices.find(relevantTimeSlice.Index);

	while (timeSliceIt != TimeSlices.end())
	{
		auto &currentTimeSlice = *timeSliceIt;

		for (auto &bpmPoint : currentTimeSlice.second.BpmPoints)
			if (bpmPoint.TimePoint > InTime)
				return &bpmPoint;

		timeSliceIt++;
	}

	return nullptr;
}

Chart::Chart()
{
	_OnModified = [](TimeSlice &InOutTimeSlice) {};
}
