using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using OpenQA.Selenium.Appium.Windows;
using OpenQA.Selenium.Remote;

namespace SamplesTest
{
    class TestHelper
    {
        private const string WindowsApplicationDriverUrl = "http://127.0.0.1:4723";

        public static WindowsDriver<WindowsElement> GetSession(String appid, String name)
        {
            WindowsDriver<WindowsElement> session;
            DesiredCapabilities appCapabilities = new DesiredCapabilities();
            appCapabilities.SetCapability("app", appid);
            appCapabilities.SetCapability("deviceName", "WindowsPC");
            try
            {
                session = new WindowsDriver<WindowsElement>(new Uri(WindowsApplicationDriverUrl), appCapabilities);
            }
            catch (System.InvalidOperationException)
            {
                // get desktop session
                DesiredCapabilities desktopAppCapabilities = new DesiredCapabilities();
                desktopAppCapabilities.SetCapability("app", "Root");
                WindowsDriver<WindowsElement> desktopSession = new WindowsDriver<WindowsElement>(new Uri(WindowsApplicationDriverUrl), desktopAppCapabilities);

                // use desktop session to locate existing app session
                WindowsElement appWindow = desktopSession.FindElementByName(name);
                String topLevelWindowHandle = appWindow.GetAttribute("NativeWindowHandle");
                topLevelWindowHandle = (int.Parse(topLevelWindowHandle)).ToString("x");
                appCapabilities = new DesiredCapabilities();
                appCapabilities.SetCapability("appTopLevelWindow", topLevelWindowHandle);
                session = new WindowsDriver<WindowsElement>(new Uri(WindowsApplicationDriverUrl), appCapabilities);
            }
            Assert.IsNotNull(session);
            return session;
        }
    }
}
