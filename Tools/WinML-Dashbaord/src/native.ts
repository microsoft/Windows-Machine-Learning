/**
 * Provide native functions. When possible, provide web replacements.
 */

import * as fs from 'fs';

export function isWeb() {
    return !fs.exists;
}

let electron: typeof Electron;
if (!isWeb()) {
    import('electron')
        .then(mod => electron = mod)
        // tslint:disable-next-line:no-console
        .catch(console.error);
}

export function showOpenDialog(options: Electron.OpenDialogOptions) {
    if (electron) {
        const { dialog } = electron.remote;
        return new Promise((resolve) => {
            dialog.showOpenDialog(options, (filePaths: string[]) => {
                resolve(filePaths);
            });
        })
    } else {
        // tslint:disable-next-line:no-console
        console.error('TODO');
        return new Promise((resolve) => resolve());
    }
}

export function showSaveDialog(options: Electron.SaveDialogOptions) {
    if (electron) {
        const { dialog } = electron.remote;
        return new Promise((resolve) => { dialog.showSaveDialog(options, resolve); } );
    } else {
        // tslint:disable-next-line:no-console
        console.error('TODO');
        return new Promise((resolve) => resolve());
    }
}
