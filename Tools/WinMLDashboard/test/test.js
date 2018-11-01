// A simple test to verify a visible window is opened with a title
var Application = require('spectron').Application
var assert = require('assert')
var path = require('path')

var appllicationPath = process.env.LOCALAPPDATA + '\\winmldashboard\\winmldashboard.exe'
console.log(appllicationPath)

var app = new Application({
  path:appllicationPath,
  startTimeout: 200000,
})

describe("test-login-form", function () {
    this.timeout(20000);
    // CSS selectors
    // const usernameInput = 'form input[formControlName="username"]';
    // const usernameError = 'form input[formControlName="username"] + div';
    // const passwordInput = 'form input[formControlName="password"]';
    // const passwordError = 'form input[formControlName="password"] + div';
    // const submitButton = 'button*=Store';
    const openModelButton = '#open-file-button';
    const modelPathInput = '#open-file-dialog'

    // Start spectron
    beforeEach(function () {
        return app.start();
    });

    // Stop Electron
    afterEach(function () {
        if (app && app.isRunning()) {
            return app.stop();
        }
    });

    describe("Edit View", function () {
        // wait for Electron window to open
        it('opens window', function () {
            return app.client.waitUntilWindowLoaded().getWindowCount().should.eventually.equal(1);
        });

        // Initially, the submit button should be deactivated
        it("opens a model to view", function () {
            return app.client
                .click(openModelButton)
                .hasFocus(modelPathInput)
                .setValue(modelPathInput, path.join(__dirname, 'la_muse.mlmodel'))
        });
    });
});