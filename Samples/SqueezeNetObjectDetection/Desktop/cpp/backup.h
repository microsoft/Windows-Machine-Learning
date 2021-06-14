#pragma once

#include "backup.g.h"

namespace winrt::SqueezeNetObjectDetection::implementation
{
    struct backup : backupT<backup>
    {
        backup();

        int32_t MyProperty();
        void MyProperty(int32_t value);

        void ClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
    };
}

namespace winrt::SqueezeNetObjectDetection::factory_implementation
{
    struct backup : backupT<backup, implementation::backup>
    {
    };
}
