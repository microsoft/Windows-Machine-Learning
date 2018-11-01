// A simple test to verify a visible window is opened with a title
var Application = require('spectron').Application
var assert = require('assert')
var path = require('path')
const chaiAsPromised = require("chai-as-promised");
const chai = require("chai");
chai.should();
chai.use(chaiAsPromised);

var appllicationPath = process.env.LOCALAPPDATA + '\\winmldashboard\\winmldashboard.exe'

var app = new Application({
  path:appllicationPath,
})

describe("WinMLDashboard Tests", function () {
    this.timeout(200000);

    // CSS selectors
    const openModelButton = '#open-file-button';
    const modelPathInput = '#open-file-dialog';
    const editButton = '#Pivot0-Tab0';
    const convertViewButton = '#Pivot0-Tab1';
    const mainView = '.MainView'
    const convertView = '.ConvertView'
    const convertButton = '#ConvertButton'
    const modelToConvertTextFiled = '#modelToConvert'

    // Start spectron
    before(function () {
        return app.start();
    });

    // Stop Electron
    after(function () {
        if (app && app.isRunning()) {
            return app.stop();
        }
    });

    describe("Edit View", function () {
        // wait for Electron window to open
        it('opens window', function () {
            return app.client
            .waitUntilWindowLoaded().getWindowCount().should.eventually.equal(1)
            .get
        });

        it('click open model button', function(){
            return app.client
                .click(openModelButton)
        })

        it('click convertButton button', function () {
            return app.client
                .click(convertViewButton)
                .element(mainView).element(convertView).should.eventually.exist
                .element(convertButton).isEnabled().should.eventually.equal(false)
        });

        it('set model path to convert', function () {
            return app.client
                .setValue(modelToConvertTextFiled, 'D:\\Windows-Machine-Learning\\Tools\\WinMLDashboard\\test\\la_muse.mlmodel')
                .getText(modelToConvertTextFiled).should.eventually.equal('D:\\Windows-Machine-Learning\\Tools\\WinMLDashboard\\test\\la_muse.mlmodel')
        });
    }); 
});