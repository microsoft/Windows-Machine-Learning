# Windows ML Dashboard

Developer dashboard for Windows ML and ONNX.

## Getting started

The dashboard can be downloaded from the [releases page](TODO). Some of the functionality can be used in the [online, browser version here](TODO).

### Building from source

#### Prerequisites

Yarn is a requirement to download dependency packages. It can be [downloaded from its official page](https://yarnpkg.com/en/docs/install). It also requires the installation of Node.js.

Yarn is also available in [Chocolatey](https://chocolatey.org/packages/yarn).

#### Building

Run `yarn` to download dependencies. Then, run one of the following:

* `yarn start` to build the project and start a local server serving its pages
* `yarn test` to run tests
* `yarn build-electron` to make a production build to be used in an Electron app
* `yarn electron-prod` to make a production build to be used in an Electron app and start the electron app

All available commands can be seen at [package.json](package.json).

### Debugging

Chromium's dev tools pane can be used for debugging. To open it in the web view, press F12. To open it in the Electron app, run it with flag `--dev-tools`, or select *View -> Toggle Dev Tools* in the application menu, or press *Ctrl+Shift+I*.

### Distribution

To distribute the application, you can copy the whole `build` folder and the files `src/electronMain.js` and `package.json` to `node_modules/electron/dist/resources/app`, and distribute the folder `node_modules/electron/dist`. The final directory structure in `node_modules/electron/dist/resources` should be:

```
app/
├───build/
├───────...
├───src/
│   └───electronMain.js
└───package.json
```

[Electron builder](https://github.com/electron-userland/electron-builder) can be used to generate installers. [See the documentation here](https://www.electron.build/).

### Built with

* [Electron](https://electronjs.org/)
* [React](https://reactjs.org/)
* [Redux](https://redux.js.org/)

### License

This tool is under the MIT license. The license can be found at the root of this repository.
