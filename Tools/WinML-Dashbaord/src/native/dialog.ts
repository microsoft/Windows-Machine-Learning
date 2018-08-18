/**
 * Provide dialogs, abstracting whether in web or Electron.
 */

import { isWeb } from './util';

let electron: typeof Electron;
if (!isWeb()) {
    import('electron')
        .then(mod => electron = mod)
        // tslint:disable-next-line:no-console
        .catch(console.error);
}

export function showWebOpenDialog(accept: string) {
    const input = document.createElement('input');
    input.setAttribute('type', 'file');
    input.setAttribute('accept', accept);
    return new Promise((resolve: any) => {
        input.addEventListener('change', () => resolve(input.files));
        input.click();
    });
}

export function showOpenDialog(options: Electron.OpenDialogOptions) {
    if (electron) {
        const { dialog } = electron.remote;
        return new Promise((resolve) => {
            dialog.showOpenDialog(options, filePaths => {
                resolve(filePaths);
            });
        })
    } else {
        // tslint:disable-next-line:no-console
        console.error('TODO');
        return Promise.resolve();
    }
}

export function showSaveDialog(options: Electron.SaveDialogOptions) {
    if (electron) {
        const { dialog } = electron.remote;
        return new Promise((resolve) => { dialog.showSaveDialog(options, resolve); } );
    } else {
        // tslint:disable-next-line:no-console
        console.error('TODO');
        return Promise.resolve();
    }
}
