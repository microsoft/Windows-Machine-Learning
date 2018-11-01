// const Application = require('spectron').Application
// const assert = require('assert')
// const electronPath = require('electron') // Require Electron from the binaries included in node_modules.
// const Path = require('path')

// describe('Application launch', function () {
//     this.timeout(10000)
//     var app = new Application({
//         path: Path.join(process.env.LOCALAPPDATA, 'WinMLDashboard/WinMLDashboard.exe')
//     })
//     it("start the applition", function () {
//         app.start().then(function () {
//             //   // Check if the window is visible
//         return app.browserWindow.isVisible()
//         })
//     })

//     it('shows an initial window', function (done) {
//         return app.client.getWindowCount().then(function (count) {
//         assert.equal(count, 1)
//         // Please note that getWindowCount() will return 2 if `dev tools` are opened.
//         // assert.equal(count, 2)
//         })
//     })

//     it('gets the tile of application', function () {
//         return app.client.getTitle().then(function (title) {
//         assert.equal(title, "WinML Dashboard")
//         // Please note that getWindowCount() will return 2 if `dev tools` are opened.
//         // assert.equal(count, 2)
//         })
//     })
// })
// const Application = require('spectron').Application
// const chai = require('chai')
// const chaiAsPromised = require('chai-as-promised')
// const electronPath = require('electron')
// const Path = require('path')

// chai.should()
// chai.use(chaiAsPromised)

// describe('Application launch', function () {
//   this.timeout(10000);

//   beforeEach(function () {
//     this.app = new Application({
//       path: Path.join(process.env.LOCALAPPDATA, 'WinMLDashboard/WinMLDashboard.exe')
//     })
//     return this.app.start()
//   })

//   beforeEach(function () {
//     chaiAsPromised.transferPromiseness = this.app.transferPromiseness
//   })

//   afterEach(function () {
//     if (this.app && this.app.isRunning()) {
//       return this.app.stop()
//     }
//   })
//   describe("basic tests", function () {
//         it('opens a window', function () {
//             return this.app.client.waitUntilWindowLoaded()
//             .getWindowCount().should.eventually.have.at.least(1)
//             .browserWindow.isMinimized().should.eventually.be.false
//             .browserWindow.isVisible().should.eventually.be.true
//             .browserWindow.isFocused().should.eventually.be.true
//             .browserWindow.getBounds().should.eventually.have.property('width').and.be.above(0)
//             .browserWindow.getBounds().should.eventually.have.property('height').and.be.above(0)
//         })

//         it('gets the tile of application', function () {
//             return app.client.getTitle().then(function (title) {
//                 assert.equal(title, "WinML Dashboard")
//                 // Please note that getWindowCount() will return 2 if `dev tools` are opened.
//                 // assert.equal(count, 2)
//                 })
//         })
//     })
// })
