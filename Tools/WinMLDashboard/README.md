# Windows ML Dashboard

Developer dashboard for Windows ML and ONNX.

## Building from source

#### Prerequisites

|Requirements|Version|Download|Command to check|
|------------|-------|--------|----------------|
|Yarn        |latest |[here](https://yarnpkg.com/en/docs/install)|`yarn --version`|
|Node.js     |latest |[here](https://nodejs.org/en/)|`node --version`|
|Python3     |3.4+   |[here](https://www.python.org/)|`python --version`|
|Git         |latest |[here](https://git-scm.com/download/win)|`git --version`|

#### Building

1. Git clone [Microsoft/Windows-Machine-Learning](https://github.com/Microsoft/Windows-Machine-Learning) repo.

2. `cd Tools/WinMLDashboard`
3. Run `Git submodule update --init --recursive` to update Netron.
4. Run `yarn` to download dependencies. 
5. Then, run one of the following:

    * `yarn start` to build and start the web application
    * `yarn electron-prod` to build and start the desktop application

> All available commands can be seen at [package.json](package.json).

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
