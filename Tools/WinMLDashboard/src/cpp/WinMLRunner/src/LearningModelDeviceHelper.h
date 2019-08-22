#pragma once
#include "CommandLineArgs.h"

struct LearningModelDeviceWithMetadata
{
    LearningModelDevice LearningModelDevice;
    DeviceType DeviceType;
    DeviceCreationLocation DeviceCreationLocation;
};

void PopulateLearningModelDeviceList(CommandLineArgs& args, std::vector<LearningModelDeviceWithMetadata>& deviceList);