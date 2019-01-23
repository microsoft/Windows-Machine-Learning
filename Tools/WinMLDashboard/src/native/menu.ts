import { ipcRenderer } from 'electron';
import { setFile, setSaveFileName } from "../datastore/actionCreators";
import { ModelProtoSingleton } from "../datastore/proto/modelProto";
import store from "../datastore/store";
import { fileFromPath, save, showNativeSaveDialog, showOpenDialog } from "./dialog";
import { isWeb } from "./util";

export function createMenu(electron: typeof Electron) {
    const { Menu } = electron.remote;
    const template = [
        {
            label: 'File',
            submenu: [
                { label: 'Open', accelerator: 'Ctrl+O', click: onOpen },
                { label: 'Open Recent', click: onOpen, enabled: false },
                { label: 'Save', accelerator: 'Ctrl+S', click: onSave },
                { label: 'Save As', accelerator: 'Ctrl+Shift+S', click: onSaveAs },
            ],
        }, {
            role: 'help',
            submenu: [
                {
                    label: 'Documentation',
                    click() {
                        const tpnUrl = 'https://github.com/Microsoft/Windows-Machine-Learning/blob/master/Tools/WinMLDashboard/README.md';
                        require('electron').shell.openExternal(tpnUrl);
                    },
                },
                { type: 'separator' },
                {
                    label: 'Report Issues/Request Features',
                    click() {
                        const tpnUrl = 'https://github.com/Microsoft/Windows-Machine-Learning/issues/new';
                        require('electron').shell.openExternal(tpnUrl);
                    },
                },
                { role: 'toggledevtools' },
                { type: 'separator' },
                {
                    label: 'Third Party Notice',
                    click() {
                        const tpnUrl = 'https://github.com/Microsoft/Windows-Machine-Learning/blob/master/Tools/WinMLDashboard/ThirdPartyNotice.txt';
                        require('electron').shell.openExternal(tpnUrl);
                    },
                },
                { type: 'separator' },
                {
                    label: "About",
                    click() {
                        ipcRenderer.send('show-about-window')
                      }
                }
            ],
        }
    ];

    if (process.platform === 'darwin') {
        template.unshift({
            label: 'WinML Dashboard',
            submenu: [
                { role: 'about' },
                { type: 'separator' },
                { role: 'services', submenu: [] },
                { type: 'separator' },
                { role: 'hide' },
                { role: 'hideothers' },
                { role: 'unhide' },
                { type: 'separator' },
                { role: 'quit '},
            ],
        } as any);
    }

    const menu = Menu.buildFromTemplate(template as any);
    store.subscribe(() => (menu.items[0] as any).submenu.items[1].enabled = !!store.getState().nodes);
    Menu.setApplicationMenu(menu);
}

export function registerKeyboardShurtcuts() {
    if (isWeb()) {  // Electron's accelerators are used outside web builds
        document.addEventListener('keydown', (event) => {
            if (event.ctrlKey) {
                switch (event.code) {
                    case 'KeyS':
                    onSave();
                    break;

                    case 'KeyO':
                    onOpen();
                    break;

                    default:
                    return;
                }
                event.preventDefault();
            }
        });
    }
}

async function onSave() {
    if (!ModelProtoSingleton.proto) {
        return;
    }
    const saveFilePath = store.getState().saveFileName || 'model.onnx';
    await save(ModelProtoSingleton.serialize(false), saveFilePath);
}

async function onSaveAs() {
    if (!ModelProtoSingleton.proto) {
        return;
    }
    const filters: Electron.FileFilter[] = [{ name: 'ONNX model', extensions: [ 'onnx', 'prototxt' ] }]
    const saveAsFilePath = await showNativeSaveDialog({ defaultPath: store.getState().saveFileName, filters });
    if (!saveAsFilePath) {
        return;
    }
    await save(ModelProtoSingleton.serialize(false), saveAsFilePath);
    store.dispatch(setFile(fileFromPath(saveAsFilePath)));
    store.dispatch(setSaveFileName(saveAsFilePath));
}

export async function onOpen() {
    const files = await showOpenDialog([
        {name: 'model', extensions: ['onnx', 'pb', 'meta', 'tflite', 'keras', 'h5', 'json', 'mlmodel', 'caffemodel']}
        // { name: 'ONNX Model', extensions: [ 'onnx', 'pb' ] },
        // { name: 'Keras Model', extensions: [ 'h5', 'json', 'keras' ] },
        // { name: 'CoreML Model', extensions: [ 'mlmodel' ] },
        // { name: 'Caffe Model', extensions: [ 'caffemodel' ] },
        // { name: 'Caffe2 Model', extensions: [ 'pb' ] },
        // { name: 'MXNet Model', extensions: [ 'model', 'json' ] },
        // { name: 'TensorFlow Graph', extensions: [ 'pb', 'meta' ] },
        // { name: 'TensorFlow Saved Model', extensions: [ 'pb' ] },
        // { name: 'TensorFlow Lite Model', extensions: [ 'tflite' ] }
    ]);
    if (files) {
        store.dispatch(setFile(files[0]));
        store.dispatch(setSaveFileName(files[0].path || files[0].name));
    }
}
