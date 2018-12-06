# WinML Dashboard

WinML Dashboard is a tool for viewing, editing, converting, and validating machine learning models for [Windows ML](https://docs.microsoft.com/en-us/windows/ai/) inference engine, which is built into Windows 10 and evaluates trained models locally on Windows devices using hardware optimizations for CPU and GPU to enable high performance inferences.   

 Today there are several different frameworks available for training and evaluating machine learning models, which makes it difficult for app developers to intergate models into their product. Windows ML uses [ONNX](http://onnx.ai/) machine learning model format that allows conversion from one framework format to another, and this Dashboard makes it easy to convert models from different framework to ONNX. 
 
 This tool supports converting to ONNX from the following source frameworks:
 - Apple Core ML
 - TensorFlow (subset of models convertible to ONNX)
 - Keras
 - Scikit-learn (subset of models convertible to ONNX)
 - Xgboost
 - LibSVM
 
The tools also validates the converted models by evaluating the model with built-in Windows ML inference engine using fake input data on CPU or GPU.

## Viewing and Editing Models

WinML Dashboard uses [Netron](https://github.com/lutzroeder/netron) for viewing machine learning model. Although WinML uses ONNX format, the Netron viewer supports viewing several different framework formats. 

Several times a developer may need to update certain model metadata, or fix inputs and outputs. This tools supports modifying model properties, metadata and input/output nodes of an ONNX model. 

Selecting Edit tab (center top as shown in the snip below) takes you to viewing and editing panel. The left pane in the panel allows editing Input and Output nodes, and the right pane allows editing Model properties. The center portion shows the graph.

The Edit/View button switches from Edit model to View only mode. View only mode enables Netron viewer with its native features. 

<img src="./public/Editor.png" height=500 width=600/>

## Converting Models

The "Convert" tab (see snip below) helps convert models from several different frameworks (as listed above) to ONNX format. 

In order to do the conversion, the tool installs a separate Python environment and a set of converter tools. This helps alleviate one of the biggest pain points of a typical developer - installing the right Python environment and tool chain for conversion. 

<img src="./public/converter.png" height=500 width=600/>

## Validating Models

Once you have an ONNX model, you can validate whether the conversion has happened successfully and that the model can be evaluated in Windows ML inference engine. This is done using the "Run" tab (see snip below).

You can choose various options such as CPU (default) vs GPU, real input vs fake input (default) etc. The result of model evaluation appears in the console window at the bottom.

<img src="./public/runview.png" height=500 width=600/>

## Build from source

#### Prerequisites

|Requirements|Version|Download|Command to check|
|------------|-------|--------|----------------|
|Python3     |3.4+   |[here](https://www.python.org/)|`python --version`|
|Yarn        |latest |[here](https://yarnpkg.com/en/docs/install)|`yarn --version`|
|Node.js     |latest |[here](https://nodejs.org/en/)|`node --version`|
|Git         |latest |[here](https://git-scm.com/download/win)|`git --version`|

> All four prerequisites should be **added to Enviroment Path**.

#### Steps to build

1. `git clone https://github.com/Microsoft/Windows-Machine-Learning`

2. `cd Tools/WinMLDashboard`
3. Run `Git submodule update --init --recursive` to update Netron.
4. Run `yarn` to download dependencies. 
5. Then, run `yarn electron-prod` to build and start the desktop application, which will launch the Dashboard.

> All available commands can be seen at [package.json](package.json).

### Debugging

To open the **debug view** in the Electron app

* Run it with flag `--dev-tools`
* Or select `View -> Toggle Dev Tools` in the application menu
* Or press `Ctrl + Shift + I`.

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