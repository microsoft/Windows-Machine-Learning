using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using System;
using System.Collections.Generic;
using System.Linq;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace WinMLSamplesGallery
{
    /// <summary>
    /// An empty window that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainWindow : Window
    {
        private readonly List<(string Tag, Type Page)> _pages = new List<(string Tag, Type Page)>
        {
            ("all_samples", typeof(AllSamplesPage)),
            ("audio", typeof(Audio)),
            ("benchmark", typeof(Benchmark)),
            ("home", typeof(HomePage)),
            ("image", typeof(Image)),
            ("video", typeof(Video)),
        };

        public MainWindow()
        {
            this.InitializeComponent();
            this.Title = "Windows AI Samples Gallery";
        }

        private void nvSample_ItemInvoked(NavigationView sender, NavigationViewItemInvokedEventArgs args)
        {
            if (args.IsSettingsInvoked == true)
            {
            }
            else if (args.InvokedItemContainer != null)
            {
                var navItemTag = args.InvokedItemContainer.Tag.ToString();
                NavView_Navigate(navItemTag, args.RecommendedNavigationTransitionInfo);
            }
        }

        private void NavView_Navigate(
            string navItemTag,
            Microsoft.UI.Xaml.Media.Animation.NavigationTransitionInfo transitionInfo)
        {
            Type _page = null;
            if (navItemTag == "settings")
            {
                // _page = typeof(SettingsPage);
            }
            else
            {
                var item = _pages.FirstOrDefault(p => p.Tag.Equals(navItemTag));
                _page = item.Page;
            }
            // Get the page type before navigation so you can prevent duplicate
            // entries in the backstack.
            var preNavPageType = contentFrame.CurrentSourcePageType;

            // Only navigate if the selected page isn't currently loaded.
            if (!(_page is null) && !Type.Equals(preNavPageType, _page))
            {
                contentFrame.Navigate(_page, null, transitionInfo);
            }
        }

        private void nvSample_BackRequested(NavigationView sender, NavigationViewBackRequestedEventArgs args)
        {
            TryGoBack();
        }

        private bool TryGoBack()
        {
            if (!contentFrame.CanGoBack)
                return false;

            // Don't go back if the nav pane is overlayed.
            if (nvSample.IsPaneOpen &&
                (nvSample.DisplayMode == NavigationViewDisplayMode.Compact ||
                 nvSample.DisplayMode == NavigationViewDisplayMode.Minimal))
                return false;

            
            contentFrame.GoBack();
            return true;
        }

        private void contentFrame_Navigated(object sender, NavigationEventArgs e)
        {
            nvSample.IsBackEnabled = contentFrame.CanGoBack;

            //if (contentFrame.SourcePageType == typeof(SettingsPage))
            //{
            //    // SettingsItem is not part of NavView.MenuItems, and doesn't have a Tag.
            //    nvSample.SelectedItem = (muxc.NavigationViewItem)nvSample.SettingsItem;
            //    nvSample.Header = "Settings";
            //}
            //else

            if (contentFrame.SourcePageType != null)
            {
                var item = _pages.FirstOrDefault(p => p.Page == e.SourcePageType);

                if (item.Tag != null)
                {
                    nvSample.SelectedItem = nvSample.MenuItems
                        .OfType<NavigationViewItem>()
                        .First(n => n.Tag.Equals(item.Tag));

                    nvSample.Header =
                        ((NavigationViewItem)nvSample.SelectedItem)?.Content?.ToString();
                }
            }
        }

        private void nvSample_Loaded(object sender, RoutedEventArgs e)
        {
            // NavView doesn't load any page by default, so load home page.
            nvSample.SelectedItem = nvSample.MenuItems.ElementAt(0);
            // If navigation occurs on SelectionChanged, then this isn't needed.
            // Because we use ItemInvoked to navigate, we need to call Navigate
            // here to load the home page.
            NavView_Navigate("home", new Microsoft.UI.Xaml.Media.Animation.EntranceNavigationTransitionInfo());

            // Listen to the window directly so the app responds
            // to accelerator keys regardless of which element has focus.
            //Microsoft.UI.Xaml.Window.Current.CoreWindow.Dispatcher.
            //    AcceleratorKeyActivated({ this, &MainPage::CoreDispatcher_AcceleratorKeyActivated });
}
    }
}
