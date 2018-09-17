using System;
using System.Collections.Generic;
using System.Drawing;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using OpenQA.Selenium.Appium.Interactions;
using OpenQA.Selenium.Appium.Windows;
using OpenQA.Selenium;
using OpenQA.Selenium.Remote;

// Define an alias to OpenQA.Selenium.Appium.Interactions.PointerInputDevice to hide
// inherited OpenQA.Selenium.Interactions.PointerInputDevice that causes ambiguity.
// In the future, all functions of OpenQA.Selenium.Appium.Interactions should be moved
// up to OpenQA.Selenium.Interactions and this alias can simply be removed.
using PointerInputDevice = OpenQA.Selenium.Appium.Interactions.PointerInputDevice;

namespace SamplesTest
{
    public class SqueezenetSession
    {
        private const string WindowsApplicationDriverUrl = "http://127.0.0.1:4723";
        // This string key is present in RegisteredUserModeAppID under AppX/vs.appxrecipe
        // TODO: this string value has to be retrieved from local test machine
        // More information on https://github.com/Microsoft/WinAppDriver
        private const string SqueezenetAppId = "9B904DD1-22BF-4715-A2D3-B0F44457074A_qzvbm97bn12kp!App";

        protected static WindowsDriver<WindowsElement> session;

        public static void Setup(TestContext context)
        {
            if (session == null)
            {
                DesiredCapabilities appCapabilities = new DesiredCapabilities();
                appCapabilities.SetCapability("app", SqueezenetAppId);
                appCapabilities.SetCapability("deviceName", "WindowsPC");
                session = new WindowsDriver<WindowsElement>(new Uri(WindowsApplicationDriverUrl), appCapabilities);
                Assert.IsNotNull(session);
                // Set implicit timeout to 1.5 seconds to make element search to retry every 500 ms for at most three times
                session.Manage().Timeouts().ImplicitWait = TimeSpan.FromSeconds(1.5);
            }
        }

        public static void TearDown()
        {
            if (session != null)
            {
                session.Quit();
                session = null;
            }
        }
    }

    [TestClass]
    public class SqueezenetTest : SqueezenetSession
    {
        private static WindowsElement loadModelButton;
        private static WindowsElement pickImageButton;
        private static WindowsElement resetButton;
        private static WindowsElement statusBlock;

        [ClassInitialize]
        public static void ClassInitialize(TestContext context)
        {
            Setup(context);
            loadModelButton = session.FindElementByName("Load model");
            pickImageButton = session.FindElementByName("Pick image");
            resetButton = session.FindElementByName("Reset");
            statusBlock = session.FindElementByAccessibilityId("StatusBlock");
        }

        [TestMethod]
        public void TestTabbyCat()
        {
            string tabbyCatPath = "kitten_224.png";
            resetButton.Click();
            System.Threading.Thread.Sleep(100);
            loadModelButton.Click();
            // wait for model to load
            System.Threading.Thread.Sleep(2000);
            pickImageButton.Click();
            // wait for file picker window to pop up
            System.Threading.Thread.Sleep(3000);
            session.Keyboard.SendKeys(tabbyCatPath);
            session.Keyboard.SendKeys(Keys.Enter);
            System.Threading.Thread.Sleep(1000);
            string result = statusBlock.Text;
            Assert.IsTrue(result.Contains("\"tabby, tabby cat\" with confidence of 0.93"));
        }
    }
}
