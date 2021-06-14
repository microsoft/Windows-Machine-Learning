#include "pch.h"
#include "backup.h"
#if __has_include("backup.g.cpp")
#include "backup.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::SqueezeNetObjectDetection::implementation
{
    backup::backup()
    {
        InitializeComponent();
    }

    int32_t backup::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void backup::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    void backup::ClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        Button().Content(box_value(L"Clicked"));
    }
}
