/**
 * Provide native menus and shortcuts. In the web, installs shortcuts only.
 */

import { setFile, setSaveFileName } from "../datastore/actionCreators";
import { ModelProtoSingleton } from "../datastore/proto/modelProto";
import store from "../datastore/store";
import { save, showOpenDialog } from "./dialog";
import { isWeb } from "./util";

export function createMenu(electron: typeof Electron) {
    const { Menu } = electron.remote;
    const template = [
        {
            label: 'File',
            submenu: [
                { label: 'Open', accelerator: 'Ctrl+O', click: onOpen },
                { label: 'Save', accelerator: 'Ctrl+S', click: onSave, enabled: false },
            ],
        }, {
            label: 'Edit',
            submenu: [
                { role: 'undo' },
                { role: 'redo' },
                { type: 'separator' },
                { role: 'cut' },
                { role: 'copy' },
                { role: 'paste' },
                { role: 'pasteandmatchstyle' },
                { role: 'delete' },
                { role: 'selectall '},
            ],
        }, {
            label: 'View',
            submenu: [
                { role: 'toggledevtools' },
                { type: 'separator' },
                { role: 'resetzoom' },
                { role: 'zoomin' },
                { role: 'zoomout' },
                { type: 'separator' },
                { role: 'togglefullscreen '},
            ],
        }, {
            role: 'help',
            submenu: [
                {
                    label: 'Third Party Notice',
                    click() {
                        const path = require('path');
                        require('electron').shell.openItem(path.join('file://', process.cwd(), 'ThirdPartyNotice.txt'));
                    },
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
    const suggestedPath = store.getState().saveFileName || 'model.onnx';
    const selectedPath = await save(ModelProtoSingleton.serialize(), suggestedPath,
        [{ name: 'ONNX model', extensions: [ 'onnx', 'prototxt' ] }]);
    if (selectedPath && selectedPath !== suggestedPath) {
        store.dispatch(setSaveFileName(suggestedPath));
    }
}

async function onOpen() {
    const files = await showOpenDialog([
        { name: 'ONNX Model', extensions: [ 'onnx', 'pb' ] },
        { name: 'Keras Model', extensions: [ 'h5', 'json', 'keras' ] },
        { name: 'CoreML Model', extensions: [ 'mlmodel' ] },
        { name: 'Caffe Model', extensions: [ 'caffemodel' ] },
        { name: 'Caffe2 Model', extensions: [ 'pb' ] },
        { name: 'MXNet Model', extensions: [ 'model', 'json' ] },
        { name: 'TensorFlow Graph', extensions: [ 'pb', 'meta' ] },
        { name: 'TensorFlow Saved Model', extensions: [ 'pb' ] },
        { name: 'TensorFlow Lite Model', extensions: [ 'tflite' ] }
    ]);
    if (files) {
        store.dispatch(setFile(files[0]));
        store.dispatch(setSaveFileName(files[0].path || files[0].name));
    }
}
