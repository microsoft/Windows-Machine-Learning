# Testing

Currently there is no public CI build yet, but there is an internal automation build maintained inside Microsoft 
to monitor the build as well as the test results daily. Our automation build assumes that test can be run on vstest.
For more information, goto [vstest](https://github.com/Microsoft/vstest). Vstest supports xUnit, msTest, nUnit, and GoogleTest adapters. 

# UI-Based Test

To contribute to creating a sample for UWP, please add a corresponding test cases to this project. 

This test is based on the assumption that winapdriver.exe is running.

Running test locally:

1. Install WinAppDriver: https://github.com/Microsoft/WinAppDriver/release
2. Run WinAppDriver.exe from the installation directory. By default IP address and port will be 127.0.0.1:4723.
3. Make sure that you create a test with a session with correct url. 

For more information on how to author appium style tests, go to https://github.com/Microsoft/WinAppDriver.
Some examples can be seen in [mnist-test](SamplesTest\MnistTest.cs)

# Non-UI Based Test

For non-UI based test, there is no requirement other than having the right adapter extensibility for vstest. Please make sure that all the 
test binaries are built into test bin folder.