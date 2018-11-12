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
        });

        it('checks title', function(){
            return app.client
                .getTitle().should.eventually.equal("WinML Dashboard")
        })

        it('clicks open model button', function(){
            return app.client
                .click(openModelButton)
        })

        it('clicks convertButton button', function () {
            return app.client
                .click(convertViewButton)
                .element(mainView).element(convertView).should.eventually.exist
        });

        it('checks python environment installation options exists', function () {
            return app.client
                .element('#ChoiceGroup17-__download').should.eventually.exist
        })
        
        it('sets model to convert', function () {
            return app.client
                .element(modelToConvertTextFiled).exist
        });
    }); 
});