#include "CommandLineArgs.h"
#include "LearningModelDeviceHelper.h"

int run(CommandLineArgs& args,
    Profiler<WINML_MODEL_TEST_PERF>& profiler,
    const std::vector<LearningModelDeviceWithMetadata>& deviceList);