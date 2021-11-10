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
            ("home", typeof(HomePage)),
            ("image", typeof(Image)),
            ("video", typeof(Video)),
        };

        public static Frame mainFrame;

        public MainWindow()
        {
            this.InitializeComponent();
            this.Title = "Windows ML Samples Gallery";
            mainFrame = contentFrame;
        }

        private void nvSample_ItemInvoked(NavigationView sender, NavigationViewItemInvokedEventArgs args)
        {
            if (args.InvokedItemContainer != null)
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
            var item = _pages.FirstOrDefault(p => p.Tag.Equals(navItemTag));
            _page = item.Page;
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
        }
    }
}
