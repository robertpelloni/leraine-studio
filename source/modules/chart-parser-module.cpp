#include "chart-parser-module.h"

#include <sstream>
#include <algorithm>
#include <limits>

#include <math.h>

#define PARSE_COMMA_VALUE(stringstream, target) stringstream >> target; if (stringstream.peek() == ',') stringstream.ignore()
#define REMOVE_POTENTIAL_NEWLINE(str) if(str.find('\r') != std::string::npos) str.resize(str.size() - 1)

void ChartParserModule::SetCurrentChartPath(const std::filesystem::path& InPath)
{
	_CurrentChartPath = InPath;
}

ChartMetadata ChartParserModule::GetChartMetadata(Chart* InChart)
{
	std::filesystem::path chartFolderPath = _CurrentChartPath;
	chartFolderPath.remove_filename();

	ChartMetadata outMetadata;

	outMetadata.Artist = InChart->Artist;
	outMetadata.SongTitle = InChart->SongTitle;
	outMetadata.Charter = InChart->Charter;
	outMetadata.DifficultyName = InChart->DifficultyName;

	outMetadata.ChartFolderPath = chartFolderPath.string();
	outMetadata.AudioPath = InChart->AudioPath;
	outMetadata.BackgroundPath = InChart->BackgroundPath;

	outMetadata.KeyAmount = InChart->KeyAmount;
	outMetadata.OD = InChart->OD;
	outMetadata.HP = InChart->HP;

	return outMetadata;
}

std::string ChartParserModule::CreateNewChart(const ChartMetadata& InNewChartData) 
{
	Chart dummyChart;

	return SetChartMetadata(&dummyChart, InNewChartData);
}

std::string ChartParserModule::SetChartMetadata(Chart* OutChart, const ChartMetadata& InChartMetadata)
{
	std::filesystem::path audioPath = InChartMetadata.AudioPath;
	std::filesystem::path backgroundPath = InChartMetadata.BackgroundPath;
	std::filesystem::path chartFolderPath = InChartMetadata.ChartFolderPath;

	std::string chartFileName = "";
	chartFileName += InChartMetadata.Artist;
	chartFileName += " - ";
	chartFileName += InChartMetadata.SongTitle;
	chartFileName += " (";
	chartFileName += InChartMetadata.Charter;
	chartFileName += ") ";
	chartFileName += "[";
	chartFileName += InChartMetadata.DifficultyName;
	chartFileName += "].osu";

	std::filesystem::path chartFilePath = chartFolderPath / chartFileName;
	std::filesystem::path targetAudioPath = chartFolderPath / audioPath.filename();
	std::filesystem::path targetBackgroundPath = chartFolderPath / backgroundPath.filename();

	if (audioPath != targetAudioPath)
		std::filesystem::copy_file(audioPath, targetAudioPath, std::filesystem::copy_options::overwrite_existing);

	if (!InChartMetadata.BackgroundPath.empty() && backgroundPath != targetBackgroundPath)
		std::filesystem::copy_file(backgroundPath, targetBackgroundPath, std::filesystem::copy_options::overwrite_existing);

	SetCurrentChartPath(chartFilePath);

	OutChart->Artist = InChartMetadata.Artist;
	OutChart->SongTitle = InChartMetadata.SongTitle;
	OutChart->Charter = InChartMetadata.Charter;
	OutChart->DifficultyName = InChartMetadata.DifficultyName;
	OutChart->AudioPath = targetAudioPath;
	OutChart->BackgroundPath = targetBackgroundPath;
	OutChart->HP = InChartMetadata.HP;
	OutChart->OD = InChartMetadata.OD;
	
	if (!OutChart->KeyAmount) 
		OutChart->KeyAmount = InChartMetadata.KeyAmount;

	ExportChartSet(OutChart);

	return chartFilePath.string();
}

Chart* ChartParserModule::ParseAndGenerateChartSet(const std::filesystem::path& InPath)
{
	std::ifstream chartFile(InPath);
	if (!chartFile.is_open()) return nullptr;
	
	_CurrentChartPath = InPath;

	PUSH_NOTIFICATION("Opened %s", InPath.c_str());

	Chart* chart = nullptr;

	if(InPath.extension() == ".osu")
		chart = ParseChartOsuImpl(chartFile, InPath);
	else if(InPath.extension() == ".sm")
		chart = ParseChartStepmaniaImpl(chartFile, InPath);

	return chart;
}

Chart* ChartParserModule::ParseChartOsuImpl(std::ifstream& InIfstream, std::filesystem::path InPath)
{
	Chart* chart = new Chart();

	std::string line;

	std::filesystem::path path = InPath;
	std::string parentPath = path.parent_path().string();

	while (std::getline(InIfstream, line))
	{
		REMOVE_POTENTIAL_NEWLINE(line);

		if (line == "[General]")
		{
			std::string metadataLine;

			while (std::getline(InIfstream, metadataLine))
			{
				REMOVE_POTENTIAL_NEWLINE(metadataLine);

				if (metadataLine.empty())
					break;

				std::string meta;
				std::string value;

				int pointer = 0;

				while (metadataLine[pointer] != ':')
					meta += metadataLine[pointer++];

				pointer++;
				pointer++;

				while (metadataLine[pointer] != '\0')
					value += metadataLine[pointer++];

				if (meta == "AudioFilename")
				{
					REMOVE_POTENTIAL_NEWLINE(value);

					std::filesystem::path resultedParentPath = parentPath;
					std::filesystem::path songPath = resultedParentPath / value;

					chart->AudioPath = songPath;
				}
			}
		}

		if (line == "[Metadata]")
		{
			std::string metadataLine;

			while (std::getline(InIfstream, metadataLine))
			{
				REMOVE_POTENTIAL_NEWLINE(metadataLine);

				if (metadataLine.empty())
					break;

				std::string meta;
				std::string value;

				int pointer = 0;

				while (metadataLine[pointer] != ':')
					meta += metadataLine[pointer++];

				pointer++;

				while (metadataLine[pointer] != '\0')
					value += metadataLine[pointer++];

				if (meta == "Title")
					chart->SongTitle = value;

				if (meta == "TitleUnicode")
					chart->SongtitleUnicode = value;

				if (meta == "Artist")
					chart->Artist = value;

				if (meta == "ArtistUnicode")
					chart->ArtistUnicode = value;

				if (meta == "Version")
					chart->DifficultyName = value;

				if (meta == "Creator")
					chart->Charter = value;

				if (meta == "Source")
					chart->Source = value;

				if (meta == "Tags")
					chart->Tags = value;

				if (meta == "BeatmapID")
					chart->BeatmapID = value;

				if (meta == "BeatmapSetID")
					chart->BeatmapSetID = value;
			}
		}

		if (line == "[Difficulty]")
		{
			std::string difficultyLine;

			while (std::getline(InIfstream, difficultyLine))
			{
				REMOVE_POTENTIAL_NEWLINE(difficultyLine);

				std::string type;
				std::string value;

				int pointer = 0;

				if (difficultyLine.empty())
					break;

				while (difficultyLine[pointer] != ':')
					type += difficultyLine[pointer++];

				pointer++;

				while (difficultyLine[pointer] != '\0')
					value += difficultyLine[pointer++];

				if (type == "CircleSize")
					chart->KeyAmount = int(std::stoi(value));

				if(type == "HPDrainRate")
					chart->HP = std::stof(value);

				if(type == "OverallDifficulty")
					chart->OD = std::stof(value);

				if (difficultyLine.empty())
					break;
			}
		}

		if (line == "[Events]")
		{
			std::string timePointLine;

			while (std::getline(InIfstream, timePointLine))
			{
				REMOVE_POTENTIAL_NEWLINE(timePointLine);

				if (timePointLine == "")
					continue;

				if (timePointLine == "[TimingPoints]")
				{
					line = "[TimingPoints]";
					break;
				}

				if (timePointLine[0] == '/' && timePointLine[1] == '/')
					continue;

				std::string background;
				int charIndex = 0;

				while (timePointLine[charIndex++] != '"');

				do
					background += timePointLine[charIndex];
				while (timePointLine[++charIndex] != '"');

				REMOVE_POTENTIAL_NEWLINE(background);

				std::filesystem::path resultedParentPath = parentPath;
				std::filesystem::path backgroundPath = resultedParentPath / background;

				chart->BackgroundPath = backgroundPath;
				
				break;
			}
		}

		if (line == "[TimingPoints]")
		{
			std::string timePointLine;
			while (InIfstream >> line)
			{
				if (line == "[HitObjects]" || line == "[Colours]")
					break;

				std::stringstream timePointStream(line);

				double timePoint;
				double beatLength;
				int meter, sampleSet, sampleIndex, volume, uninherited, effects;

				PARSE_COMMA_VALUE(timePointStream, timePoint);
				PARSE_COMMA_VALUE(timePointStream, beatLength);
				PARSE_COMMA_VALUE(timePointStream, meter);
				PARSE_COMMA_VALUE(timePointStream, sampleSet);
				PARSE_COMMA_VALUE(timePointStream, sampleIndex);
				PARSE_COMMA_VALUE(timePointStream, volume);
				PARSE_COMMA_VALUE(timePointStream, uninherited);
				PARSE_COMMA_VALUE(timePointStream, effects);

				//BPMData* bpmData = new BPMData();
				//bpmData->BPMSaved = bpm;

				if (beatLength < 0)
				{
					chart->InheritedTimingPoints.push_back(timePointLine);
					continue;
				}

				double bpm = 60000.0 / beatLength;

				//bpmData->BPM = bpm;
				//bpmData->timePoint = int(timePoint);
				//bpmData->meter = meter;
				//bpmData->uninherited = uninherited;

				chart->InjectBpmPoint(Time(timePoint), bpm, beatLength);
			}
		}

		if (line == "[HitObjects]")
		{
			std::string noteLine;
			while (std::getline(InIfstream, noteLine))
			{
				REMOVE_POTENTIAL_NEWLINE(noteLine);

				if (noteLine == "")
					continue;

				std::stringstream noteStream(noteLine);
				int column, y, timePoint, noteType, hitSound, timePointEnd;

				PARSE_COMMA_VALUE(noteStream, column);
				PARSE_COMMA_VALUE(noteStream, y);
				PARSE_COMMA_VALUE(noteStream, timePoint);
				PARSE_COMMA_VALUE(noteStream, noteType);
				PARSE_COMMA_VALUE(noteStream, hitSound);
				PARSE_COMMA_VALUE(noteStream, timePointEnd);

				int parsedColumn = std::clamp(floor(float(column) * (float(chart->KeyAmount) / 512.f)), 0.f, float(chart->KeyAmount) - 1.f);

				if (noteType == 128)
				{
					chart->InjectHold(timePoint, timePointEnd, parsedColumn);
				}
				else if (noteType == 1 || noteType == 5)
				{
					chart->InjectNote(timePoint, parsedColumn, Note::EType::Common);
				}
			}
		}
	}


	return chart;
}

// Helper to keep track of holds
struct SmHoldTracker
{
	Time TimePointBegin;
	Column Col;
    Note::EType Type;
};

// Helper struct for beat-based bpm points needed during parsing
struct SmBpmPoint
{
	double Beat;
	double Bpm;
};

Chart* ChartParserModule::ParseChartStepmaniaImpl(std::ifstream& InIfstream, std::filesystem::path InPath)
{
	Chart* chart = new Chart();
	chart->KeyAmount = 4; // Default to 4 keys for dance-single

	std::string line;
	std::string buffer; // To accumulate multiline values

	double offset = 0.0;
	std::vector<SmBpmPoint> smBpmPoints;
    struct SmStop
    {
        double Beat;
        double Length;
    };
    std::vector<SmStop> smStops;
	bool inNotes = false;

	// Helper to calculate time from beat using smBpmPoints and offset
	auto GetTimeFromBeat = [&](double InBeat) -> Time
	{
		double time = offset * 1000.0;
		double currentBeat = 0.0;

		if (smBpmPoints.empty()) return Time(time);

		double currentBpm = smBpmPoints[0].Bpm;

		for (size_t i = 0; i < smBpmPoints.size(); ++i)
		{
			double nextBeat = (i + 1 < smBpmPoints.size()) ? smBpmPoints[i+1].Beat : InBeat;

			// If our target beat is before the next change, we stop here
			if (InBeat < nextBeat)
			{
				time += (InBeat - currentBeat) * (60000.0 / currentBpm);
				return Time(time);
			}

			// Add time for this segment
			time += (nextBeat - currentBeat) * (60000.0 / currentBpm);
			currentBeat = nextBeat;
			currentBpm = (i + 1 < smBpmPoints.size()) ? smBpmPoints[i+1].Bpm : currentBpm;
		}

		// If we are past the last defined BPM change
		if (InBeat > currentBeat)
		{
			time += (InBeat - currentBeat) * (60000.0 / currentBpm);
		}

		return Time(time);
	};

	std::filesystem::path path = InPath;
	std::string parentPath = path.parent_path().string();

	while (std::getline(InIfstream, line))
	{
		// Basic comment stripping
		size_t commentPos = line.find("//");
		if (commentPos != std::string::npos)
			line = line.substr(0, commentPos);

		// Trim
		line.erase(0, line.find_first_not_of(" \t\r\n"));
		line.erase(line.find_last_not_of(" \t\r\n") + 1);

		if (line.empty()) continue;

		if (line[0] == '#')
		{
			// Tag parsing
			size_t colonPos = line.find(':');
			if (colonPos != std::string::npos)
			{
				std::string key = line.substr(1, colonPos - 1);
				std::string value = line.substr(colonPos + 1);

				// Read until semicolon if not present
				while (value.find(';') == std::string::npos && InIfstream.peek() != EOF)
				{
					std::string nextLine;
					std::getline(InIfstream, nextLine);
					value += nextLine + "\n";
				}

				if (!value.empty() && value.back() == ';') value.pop_back();

				if (key == "TITLE") chart->SongTitle = value;
				else if (key == "ARTIST") chart->Artist = value;
				else if (key == "CREDIT") chart->Charter = value;
				else if (key == "MUSIC")
				{
					std::filesystem::path songPath = std::filesystem::path(parentPath) / value;
					chart->AudioPath = songPath;
				}
				else if (key == "BANNER" || key == "BACKGROUND")
				{
					std::filesystem::path bgPath = std::filesystem::path(parentPath) / value;
					chart->BackgroundPath = bgPath;
				}
				else if (key == "OFFSET")
				{
					offset = std::stod(value);
				}
				else if (key == "BPMS")
				{
					// beat=bpm, beat=bpm
					std::string pairStr;
					std::stringstream ss(value);
					while (std::getline(ss, pairStr, ','))
					{
						size_t eqPos = pairStr.find('=');
						if (eqPos != std::string::npos)
						{
							double beat = std::stod(pairStr.substr(0, eqPos));
							double bpm = std::stod(pairStr.substr(eqPos + 1));
							smBpmPoints.push_back({beat, bpm});
						}
					}
					// Ensure sorted
					std::sort(smBpmPoints.begin(), smBpmPoints.end(), [](const SmBpmPoint& a, const SmBpmPoint& b){ return a.Beat < b.Beat; });

					// Inject into Chart
					for (const auto& pt : smBpmPoints)
					{
						Time t = GetTimeFromBeat(pt.Beat);
						chart->InjectBpmPoint(t, pt.Bpm, 60000.0 / pt.Bpm);
					}
				}
                else if (key == "STOPS")
                {
                    std::string pairStr;
                    std::stringstream ss(value);
                    while (std::getline(ss, pairStr, ','))
                    {
                        size_t eqPos = pairStr.find('=');
                        if (eqPos != std::string::npos)
                        {
                            double beat = std::stod(pairStr.substr(0, eqPos));
                            double len = std::stod(pairStr.substr(eqPos + 1));
                            smStops.push_back({beat, len});
                        }
                    }
                }
                else if (key == "BGCHANGES") chart->SmBgChanges = value;
                else if (key == "FGCHANGES") chart->SmFgChanges = value;
				else if (key == "NOTES")
				{
					inNotes = true;
					// Notes format is complex, we need to read section by section
					// The #NOTES line itself usually doesn't contain data, the data follows
					// But our logic above might have slurped it into `value`.

					// Re-construct the buffer to parse specific note fields
					buffer = value;
				}
			}
		}

		if (inNotes)
		{
			// Parse Note Data
			// Structure: Type:Desc:Diff:Meter:Radar:Data
			std::vector<std::string> sections;
			std::stringstream ss(buffer);
			std::string segment;

			// We might have multiple charts in one file.
			// Current logic just takes the buffer which contains *one* chart definition (until semicolon)
			// But wait, my slurping logic above stopped at ';'.
			// StepMania #NOTES ends with ';'.

			// Split by ':'
			while (std::getline(ss, segment, ':'))
			{
				// trim segment
				segment.erase(0, segment.find_first_not_of(" \t\r\n"));
				segment.erase(segment.find_last_not_of(" \t\r\n") + 1);
				sections.push_back(segment);
			}

			if (sections.size() >= 6)
			{
                for (const auto& stop : smStops)
                {
                    Time t = GetTimeFromBeat(stop.Beat);
                    chart->InjectStop(t, stop.Length);
                }

				std::string chartType = sections[0]; // dance-single
				// Only parse dance-single for now
				if (chartType.find("dance-single") != std::string::npos)
				{
					chart->DifficultyName = sections[2]; // Difficulty Class (Hard, etc)

					// Note Data is in sections[5] (and subsequent if split failed on colons inside data, but data usually doesn't have colons)
					// Actually, sections might be wrong if data contains colons? No, data uses 0123M and newlines/commas.

					std::string noteData = sections[5];
					// If there were more colons, append them back? unlikely for standard SM.

					// Process Measures
					std::vector<SmHoldTracker> holds;
					double currentMeasureIndex = 0;

					std::stringstream measureStream(noteData);
					std::string measureStr;

					while (std::getline(measureStream, measureStr, ','))
					{
						// Process one measure
						// Split into rows
						std::vector<std::string> rows;
						std::stringstream rowStream(measureStr);
						std::string rowStr;
						while (std::getline(rowStream, rowStr))
						{
							// Clean row
							rowStr.erase(0, rowStr.find_first_not_of(" \t\r"));
							rowStr.erase(rowStr.find_last_not_of(" \t\r") + 1);
							// Remove comments in note data?
							if (rowStr.substr(0, 2) == "//") continue;

							// Check for semicolon terminator
							size_t semiPos = rowStr.find(';');
							if (semiPos != std::string::npos)
							{
								rowStr = rowStr.substr(0, semiPos);
								if (rowStr.empty())
								{
									// Semicolon found on empty line (or start of line).
									break;
								}
							}

							if (rowStr.empty()) continue;
							rows.push_back(rowStr);
						}

						int numRows = rows.size();
						if (numRows == 0)
						{
							currentMeasureIndex++;
							continue;
						}

						for (int r = 0; r < numRows; ++r)
						{
							double beatIndex = (currentMeasureIndex * 4.0) + ((double)r / (double)numRows) * 4.0;
							Time t = GetTimeFromBeat(beatIndex);

							std::string& row = rows[r];
							for (int c = 0; c < 4 && c < (int)row.size(); ++c) // 4 columns
							{
								char type = row[c];
								if (type == '1') // Tap
								{
									chart->InjectNote(t, c, Note::EType::Common);
								}
								else if (type == '2') // Hold Head
								{
									holds.push_back({t, (Column)c, Note::EType::HoldBegin});
								}
                                else if (type == '4') // Roll Head
                                {
                                    holds.push_back({t, (Column)c, Note::EType::RollBegin});
                                }
								else if (type == '3') // Hold Tail
								{
									// Find matching head
									for (auto it = holds.begin(); it != holds.end(); ++it)
									{
										if (it->Col == (Column)c)
										{
                                            if (it->Type == Note::EType::HoldBegin)
											    chart->InjectHold(it->TimePointBegin, t, c);
                                            else if (it->Type == Note::EType::RollBegin)
                                                chart->InjectRoll(it->TimePointBegin, t, c);

											holds.erase(it);
											break;
										}
									}
								}
								else if (type == 'M') // Mine
								{
									chart->InjectNote(t, c, Note::EType::Mine);
								}
                                else if (type == 'L') // Lift
                                {
                                    chart->InjectNote(t, c, Note::EType::Lift);
                                }
                                else if (type == 'F') // Fake
                                {
                                    chart->InjectNote(t, c, Note::EType::Fake);
                                }
							}
						}
						currentMeasureIndex++;
					}

					// Found and parsed one chart, we return it.
					// If file has multiple difficulties, this picks the first dance-single one.
					return chart;
				}
			}

			// If not dance-single or failed parse, reset inNotes to look for next #NOTES
			inNotes = false;
			buffer.clear();
		}
	}

	return chart;
}

void ChartParserModule::ExportChartSet(Chart* InChart)
{
	std::ofstream chartFile(_CurrentChartPath);

	ExportChartOsuImpl(InChart, chartFile);

	PUSH_NOTIFICATION("Saved to %s", _CurrentChartPath.c_str());
}

void ChartParserModule::ExportChartOsuImpl(Chart* InChart, std::ofstream& InOfStream)
{	
	std::string backgroundFileName = InChart->BackgroundPath.filename().string();
	std::string audioFileName = InChart->AudioPath.filename().string();
	
	std::stringstream chartStream;

	chartStream << "osu file format v14" << "\n"
						  << "\n"
						  << "[General]" << "\n"
						  << "AudioFilename: " << audioFileName << "\n"
						  << "AudioLeadIn: 0" << "\n"
						  << "PreviewTime: 0" << "\n"
						  << "Countdown: 0" << "\n"
						  << "SampleSet: Soft" << "\n"
						  << "StackLeniency: 0.7" << "\n"
						  << "Mode: 3" << "\n"
						  << "LetterboxInBreaks: 0" << "\n"
						  << "SpecialStyle: 0" << "\n"
						  << "WidescreenStoryboard: 0" << "\n"
						  << "\n"
						  << "[Editor]" << "\n"
						  << "DistanceSpacing: 1" << "\n"
						  << "BeatDivisor: 4" << "\n"
						  << "GridSize: 16" << "\n"
						  << "TimelineZoom: 1" << "\n"
						  << "\n"
						  << "[Metadata]" << "\n"
						  << "Title:" << InChart->SongTitle << "\n"
						  << "TitleUnicode:" << InChart->SongtitleUnicode << "\n"
						  << "Artist:" << InChart->Artist << "\n"
						  << "ArtistUnicode:" << InChart->ArtistUnicode << "\n"
						  << "Creator:" << InChart->Charter << "\n"
						  << "Version:" << InChart->DifficultyName << "\n"
						  << "Source:" << InChart->Source << "\n"
						  << "Tags:" << InChart->Tags <<  "\n"
						  << "BeatmapID:" << InChart->BeatmapID << "\n"
						  << "BeatmapSetID:" << InChart->BeatmapSetID << "\n"
						  << "\n"
						  << "[Difficulty]" << "\n"
						  << "HPDrainRate:" << InChart->HP << "\n"
						  << "CircleSize:" << InChart->KeyAmount << "\n"
						  << "OverallDifficulty:" << InChart->OD << "\n"
						  << "ApproachRate:9" << "\n"
						  << "SliderMultiplier:1.4" << "\n"
						  << "SliderTickRate:1" << "\n"
						  << "\n"
						  << "[Events]" << "\n"
							<< "//Background and Video events" << "\n";

	if (backgroundFileName != "")
		chartStream << "0,0,\"" << backgroundFileName << "\",0,0" << "\n";

	chartStream << "//Break Periods" << "\n"
							<< "//Storyboard Layer 0 (Background)" << "\n"
							<< "//Storyboard Layer 1 (Fail)" << "\n"
							<< "//Storyboard Layer 2 (Pass)" << "\n"
							<< "//Storyboard Layer 3 (Foreground)" << "\n"
							<< "//Storyboard Layer 4 (Overlay)" << "\n"
							<< "//Storyboard Sound Samples" << "\n"
							<< "\n"
							<< "[TimingPoints]" << "\n";

	for (std::string inheritedPoint : InChart->InheritedTimingPoints)
		chartStream << inheritedPoint;
	
	InChart->IterateAllBpmPoints([&chartStream](BpmPoint& InBpmPoint)
	{
		chartStream << InBpmPoint.TimePoint << "," << InBpmPoint.BeatLength << "," << "4" << ",0,0,10,1,0\n";
	});

	// leaving the "4" there since we will want to set custom snap divisor
	
	chartStream << "\n"
							<< "\n"
							<< "[HitObjects]" << "\n";

	int keyAmount = InChart->KeyAmount;
	InChart->IterateAllNotes([keyAmount, &chartStream](const Note InOutNote, const Column InColumn)
	{
		int column = float(float((InColumn + 1)) * 512.f) / float(keyAmount) - (512.f / float(keyAmount) / 2.f);

		switch (InOutNote.Type)
		{
		case Note::EType::Common:
			chartStream << column << ",192," << InOutNote.TimePoint << ",1,0,0:0:0:0:\n";
			break;
		
		case Note::EType::HoldBegin:
			chartStream << column << ",192," << InOutNote.TimePoint << ",128,0," << InOutNote.TimePointEnd << ":0:0:0:0:\n";
			break;

		default:
			break;
		}
	});

	InOfStream.clear();
	InOfStream << chartStream.str();
	InOfStream.close();
}

void ChartParserModule::ExportChartStepmaniaImpl(Chart* InChart, std::ofstream& InOfStream)
{
	// 1. Calculate Beats for all BPM Points to generate #BPMS and for note conversion
	struct ProcessedBpmPoint
	{
		double Beat;
		double Bpm;
		Time TimePoint;
	};
	std::vector<ProcessedBpmPoint> processedBpmPoints;

	// Sort BPM points from Chart
	std::vector<BpmPoint> sortedBpmPoints;
	InChart->IterateAllBpmPoints([&sortedBpmPoints](BpmPoint& pt){ sortedBpmPoints.push_back(pt); });
	std::sort(sortedBpmPoints.begin(), sortedBpmPoints.end(), [](const BpmPoint& a, const BpmPoint& b){ return a.TimePoint < b.TimePoint; });

	// Offset logic:
	// SM Offset is "Time of Beat 0".
	// If the first note or BPM point is at 1000ms, and that is Beat 0, then Offset = 1.0.
	// We need to decide where "Beat 0" is.
	// Usually, the first BPM point determines the grid.
	// If sortedBpmPoints[0].TimePoint is 1000ms, let's say that's Beat 0.
	// Then Offset = sortedBpmPoints[0].TimePoint / 1000.0.

	double offset = 0.0;
	if (!sortedBpmPoints.empty())
		offset = sortedBpmPoints[0].TimePoint / 1000.0;

	double currentBeat = 0.0;
	double lastTime = offset * 1000.0; // This is Time of Beat 0
	double currentBpm = 120.0;

	if (!sortedBpmPoints.empty())
	{
		// Initial BPM is the one at Beat 0 (or the first one)
		currentBpm = sortedBpmPoints[0].Bpm;
		processedBpmPoints.push_back({0.0, currentBpm, (Time)lastTime});
	}

	for (size_t i = 1; i < sortedBpmPoints.size(); ++i)
	{
		double timeDelta = sortedBpmPoints[i].TimePoint - lastTime;
		double beatDelta = timeDelta / (60000.0 / currentBpm);
		currentBeat += beatDelta;

		processedBpmPoints.push_back({currentBeat, sortedBpmPoints[i].Bpm, sortedBpmPoints[i].TimePoint});

		lastTime = sortedBpmPoints[i].TimePoint;
		currentBpm = sortedBpmPoints[i].Bpm;
	}

	auto TimeToBeat = [&](Time t) -> double {
		// Find relevant BPM segment
		// processedBpmPoints has (Beat, Bpm, TimePoint).
		// We want to map Time t -> Beat.

		if (processedBpmPoints.empty()) return 0.0;

		// Find the segment that starts <= t
		int idx = -1;
		for (int i = processedBpmPoints.size() - 1; i >= 0; --i)
		{
			if (processedBpmPoints[i].TimePoint <= t + 1) // +1 epsilon
			{
				idx = i;
				break;
			}
		}

		if (idx == -1) // Time is before first BPM point (before offset?)
		{
			// Extrapolate backwards using first BPM
			double delta = t - processedBpmPoints[0].TimePoint;
			return processedBpmPoints[0].Beat + delta / (60000.0 / processedBpmPoints[0].Bpm);
		}

		double delta = t - processedBpmPoints[idx].TimePoint;
		return processedBpmPoints[idx].Beat + delta / (60000.0 / processedBpmPoints[idx].Bpm);
	};

	// 2. Write Header
	std::stringstream ss;
	ss << "#TITLE:" << InChart->SongTitle << ";\n";
	ss << "#SUBTITLE:;\n";
	ss << "#ARTIST:" << InChart->Artist << ";\n";
	ss << "#TITLETRANSLIT:" << InChart->SongtitleUnicode << ";\n";
	ss << "#ARTISTTRANSLIT:" << InChart->ArtistUnicode << ";\n";
	ss << "#GENRE:;\n";
	ss << "#CREDIT:" << InChart->Charter << ";\n";
	ss << "#MUSIC:" << InChart->AudioPath.filename().string() << ";\n";
	ss << "#BANNER:" << InChart->BackgroundPath.filename().string() << ";\n";
	ss << "#BACKGROUND:;\n";
	ss << "#LYRICSPATH:;\n";
	ss << "#CDTITLE:;\n";
	ss << "#OFFSET:" << offset << ";\n";
	ss << "#SAMPLESTART:0.000;\n";
	ss << "#SAMPLELENGTH:10.000;\n";
	ss << "#SELECTABLE:YES;\n";

	ss << "#BPMS:";
	for (size_t i = 0; i < processedBpmPoints.size(); ++i)
	{
		ss << processedBpmPoints[i].Beat << "=" << processedBpmPoints[i].Bpm;
		if (i < processedBpmPoints.size() - 1) ss << ",";
		else ss << ";\n";
	}

	ss << "#STOPS:";
    std::vector<StopPoint> stops;
    InChart->IterateAllStops([&](StopPoint& s){ stops.push_back(s); });
    std::sort(stops.begin(), stops.end(), [](const StopPoint& a, const StopPoint& b){ return a.TimePoint < b.TimePoint; });

    for (size_t i = 0; i < stops.size(); ++i)
    {
        double beat = TimeToBeat(stops[i].TimePoint);
        ss << beat << "=" << stops[i].Length;
        if (i < stops.size() - 1) ss << ",";
    }
	ss << ";\n";

	ss << "#BGCHANGES:" << InChart->SmBgChanges << ";\n";
	ss << "#FGCHANGES:" << InChart->SmFgChanges << ";\n";

	// 3. Convert Notes to Beat Positions
	struct SmNote
	{
		double Beat;
		int Column;
		char Type; // 1=Tap, 2=Head, 3=Tail
	};
	std::vector<SmNote> smNotes;

	InChart->IterateAllNotes([&](Note& n, const Column& col) {
		// Only support 4 keys for now
		if (col >= 4) return;

		double b = TimeToBeat(n.TimePoint);
		if (n.Type == Note::EType::Common)
		{
			smNotes.push_back({b, (int)col, '1'});
		}
		else if (n.Type == Note::EType::HoldBegin)
		{
			smNotes.push_back({b, (int)col, '2'});
			double bEnd = TimeToBeat(n.TimePointEnd);
			smNotes.push_back({bEnd, (int)col, '3'});
		}
        else if (n.Type == Note::EType::RollBegin)
        {
            smNotes.push_back({b, (int)col, '4'});
            double bEnd = TimeToBeat(n.TimePointEnd);
            smNotes.push_back({bEnd, (int)col, '3'});
        }
        else if (n.Type == Note::EType::Mine)
        {
            smNotes.push_back({b, (int)col, 'M'});
        }
        else if (n.Type == Note::EType::Lift)
        {
            smNotes.push_back({b, (int)col, 'L'});
        }
        else if (n.Type == Note::EType::Fake)
        {
            smNotes.push_back({b, (int)col, 'F'});
        }
	});

	std::sort(smNotes.begin(), smNotes.end(), [](const SmNote& a, const SmNote& b){
		if (std::abs(a.Beat - b.Beat) > 0.001) return a.Beat < b.Beat;
		return a.Column < b.Column;
	});

	// 4. Write #NOTES
	ss << "//---------------" << InChart->DifficultyName << " - " << InChart->Charter << "---------------\n";
	ss << "#NOTES:\n";
	ss << "     dance-single:\n";
	ss << "     " << InChart->Charter << ":\n";
	ss << "     " << InChart->DifficultyName << ":\n"; // Difficulty Class needs mapping? Or just use name
	ss << "     8:\n"; // Meter hardcoded for now
	ss << "     0.000,0.000,0.000,0.000,0.000:\n";

	// 5. Write Measures
	int currentMeasure = 0;
	size_t noteIdx = 0;

	// Determine last measure
	double lastBeat = smNotes.empty() ? 0.0 : smNotes.back().Beat;
	int totalMeasures = (int)ceil(lastBeat / 4.0);
	if (totalMeasures == 0 && !smNotes.empty()) totalMeasures = 1;

	for (int m = 0; m < totalMeasures; ++m)
	{
		// Find notes in this measure [m*4, (m+1)*4)
		std::vector<SmNote*> measureNotes;
		while (noteIdx < smNotes.size() && smNotes[noteIdx].Beat < (m + 1) * 4)
		{
			if (smNotes[noteIdx].Beat >= m * 4 - 0.001) // Tolerance
				measureNotes.push_back(&smNotes[noteIdx]);
			noteIdx++;
		}

		// Determine quantization
		int divs[] = {4, 8, 12, 16, 24, 32, 48, 64, 192};
		int bestDiv = 4;

		for (int div : divs)
		{
			bool fit = true;
			for (auto* n : measureNotes)
			{
				double relativeBeat = n->Beat - (m * 4);
				double pos = relativeBeat * div;
				// Check if pos is close to integer
				if (std::abs(pos - std::round(pos)) > 0.01)
				{
					fit = false;
					break;
				}
			}
			if (fit)
			{
				bestDiv = div;
				break;
			}
			bestDiv = 192; // Fallback
		}

		// Write rows
		for (int r = 0; r < bestDiv; ++r)
		{
			char rowStr[5] = "0000";

			// Check for notes at this row
			double rowBeatStart = (m * 4) + (double)r / bestDiv * 4.0;
			// We check for notes approximately at this beat

			for (auto* n : measureNotes)
			{
				if (std::abs(n->Beat - rowBeatStart) < 0.01) // Tolerance
				{
					if (n->Column < 4)
						rowStr[n->Column] = n->Type;
				}
			}

			ss << rowStr << "\n";
		}

		if (m < totalMeasures - 1)
			ss << ",\n";
		else
			ss << ";\n";
	}

	InOfStream.clear();
	InOfStream << ss.str();
	InOfStream.close();
}
