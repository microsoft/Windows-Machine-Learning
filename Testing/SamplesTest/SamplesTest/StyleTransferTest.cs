using System;
using System.Collections.Generic;
using System.Drawing;
using System.Threading;
using System.Threading.Tasks;
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
    public enum Style
    {
        Candy,
        Mosaic,
        La_muse,
        Udnie
    }

    public class StyleTransferSession
    {
        private const string WindowsApplicationDriverUrl = "http://127.0.0.1:4723";
        // This string key is present in RegisteredUserModeAppID under AppX/vs.appxrecipe
        // TODO: this string value has to be retrieved from local test machine
        // More information on https://github.com/Microsoft/WinAppDriver
        private const string StyleTransferAppId = "257918C9-ABE3-483E-A202-C5C69AEBD825_adth261zac3sy!App";

        protected static WindowsDriver<WindowsElement> session;
        protected static WindowsElement resultImage;
        protected static WindowsElement statusBlock;

        protected static int styleTransferTimeout = 5000; // maximum time allowed for style transfer in milliseconds

        protected static IDictionary<Style, WindowsElement> styleElements;

        public static void Setup(TestContext context)
        {
            if (session == null)
            {
                DesiredCapabilities appCapabilities = new DesiredCapabilities();
                appCapabilities.SetCapability("app", StyleTransferAppId);
                appCapabilities.SetCapability("deviceName", "WindowsPC");
                session = new WindowsDriver<WindowsElement>(new Uri(WindowsApplicationDriverUrl), appCapabilities);
                Assert.IsNotNull(session);
                // Set implicit timeout to 1.5 seconds to make element search to retry every 500 ms for at most three times
                session.Manage().Timeouts().ImplicitWait = TimeSpan.FromSeconds(1.5);
            }
            styleElements = new Dictionary<Style, WindowsElement>();
            foreach(Style style in (Style[])Enum.GetValues(typeof(Style)))
            {
                styleElements[style] = session.FindElementByName(style.ToString().ToLower());
            }
            resultImage = session.FindElementByAccessibilityId("UIResultImage");
            statusBlock = session.FindElementByAccessibilityId("StatusBlock");
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
    public class StyleTransferTest : StyleTransferSession
    {

        [ClassInitialize]
        public static void ClassInitialize(TestContext context)
        {
            Setup(context);
        }

        [TestMethod]
        public void StyleCandy()
        {
            TestStyle(Style.Candy);
        }

        [TestMethod]
        public void StyleMosaic()
        {
            TestStyle(Style.Mosaic);
        }

        [TestMethod]
        public void StyleUdnie()
        {
            TestStyle(Style.Udnie);
        }

        [TestMethod]
        public void StyleLa_Muse()
        {
            TestStyle(Style.La_muse);
        }

        public void TestStyle(Style style)
        {
            styleElements[style].Click();
            var cts = new CancellationTokenSource();

            var styleTransferTask = Task.Factory.StartNew(state =>
            {
                styleElements[style].Click();
                var token = (CancellationToken)state;
                while (statusBlock.Text != "Done!")
                {
                    if (token.IsCancellationRequested)
                    {
                        throw new OperationCanceledException(token);
                    }
                }
            }, cts.Token, cts.Token);

            if (!styleTransferTask.Wait(styleTransferTimeout, cts.Token)) {
                cts.Cancel();
                Assert.Fail("style transfer timed out");
            }
            string resultPath = String.Format("Resource\\result-{0}.png", style.ToString());
            string baselinePath = String.Format("Resource\\baseline-{0}.png", style.ToString());
            // get a screenshot for result
            var screenShot = resultImage.GetScreenshot();
            screenShot.SaveAsFile(resultPath, ScreenshotImageFormat.Png);

            // compare images
            CompareImages(resultPath, baselinePath);
        }

        public void CompareImages(string resultPath, string baselinePath)
        {
            byte[] result = ImageToByteArray(resultPath);
            byte[] expected = ImageToByteArray(baselinePath);

            var maxPixelErrorsAllowed = 0.01; // maximum percentage of pixels allowed to be different
            int pixelTolerance = 20; // minimum pixel value difference to mark two pixel as "different"
            int numPixelErrors = 0;
            for (int i = 0; i < result.Length; i++)
            {
                if (Math.Abs(result[i] - expected[i]) > pixelTolerance)
                {
                    numPixelErrors++;
                    if ((numPixelErrors / (float)result.Length) > maxPixelErrorsAllowed)
                    {
                        Assert.Fail("the result is different from what's expected");
                    }
                }
            }
        }

        public byte[] ImageToByteArray(string imagePath)
        {
            Image img = Image.FromFile(imagePath);
            var ms = new System.IO.MemoryStream();
            img.Save(ms, img.RawFormat);
            return ms.ToArray();
        }
    }
}
