#pragma once

#include "base/module.h"

#include <fstream>
#include <filesystem>
#include <vector>

#include "../structures/chart-metadata.h"


/*
* TODO: see over what variables we can change to std::filesystem specifics and look over what data we can save in the chart itself
*/

struct ChartDefinition
{
    std::string DifficultyName;
    std::string Creator;
    std::string ChartType;
};

class ChartParserModule : public Module
{
public:

    std::vector<ChartDefinition> ScanForCharts(const std::filesystem::path& InPath);
    Chart* LoadChart(const std::filesystem::path& InPath, const std::string& InDifficultyName = "");

	Chart* ParseAndGenerateChartSet(const std::filesystem::path& InPath);
	void ExportChartSet(Chart* InChart);

	void SetCurrentChartPath(const std::filesystem::path& InPath);

	ChartMetadata GetChartMetadata(Chart* InChart);

	//this is for now osu impl only, in the future I'll make some template magic for which format is present
	std::string SetChartMetadata(Chart* Outchart, const ChartMetadata& InMetadata);
	std::string CreateNewChart(const ChartMetadata& InNewChartData);
private:

	std::filesystem::path _CurrentChartPath;

	Chart* ParseChartOsuImpl(std::ifstream& InIfstream, std::filesystem::path InPath);
	Chart* ParseChartStepmaniaImpl(std::ifstream& InIfstream, std::filesystem::path InPath, const std::string& InDifficultyName = "");

	void ExportChartOsuImpl(Chart* InChart, std::ofstream& InOfStream);
	void ExportChartStepmaniaImpl(Chart* InChart, std::ofstream& InOfStream);
};
