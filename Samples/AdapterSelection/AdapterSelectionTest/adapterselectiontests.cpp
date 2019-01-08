#include "stdafx.h"
#include "CppUnitTest.h"
#include "../AdapterSelection/cpp/AdapterSelection.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace winrt;
using namespace std;

namespace AdapterSelectionTest
{		
	TEST_CLASS(UnitTest1)
	{
	public:
		
		TEST_METHOD(EnumerateAdaptersTest)
		{
            vector<com_ptr<IDXGIAdapter1>> adapters = AdapterSelection::EnumerateAdapters(true);
            // TODO: Your test code here
		}

        TEST_METHOD(LearningModelDeviceFromAdapter)
        {

        }
	};
}