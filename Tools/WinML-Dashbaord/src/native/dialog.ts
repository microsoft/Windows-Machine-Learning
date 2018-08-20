/**
 * Provide dialogs, abstracting whether in web or Electron.
 */

import * as fs from 'fs';
import * as path from 'path';

import { isWeb } from './util';

let electron: typeof Electron;
if (!isWeb()) {
    import('electron')
        .then(mod => electron = mod)
        // tslint:disable-next-line:no-console
        .catch(console.error);
}

export function filterToAccept(filters: Electron.FileFilter[]) {
    /**
     * Convert Electron FileFilter[] to HTML accept field.
     */
    return filters.map(x => x.extensions.map(extension => `.${extension}`).join(',')).join(',');
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

export function showNativeOpenDialog(options: Electron.OpenDialogOptions) {
    const { dialog } = electron.remote;
    return new Promise(resolve => dialog.showOpenDialog(options, resolve));
}

export function showOpenDialog(filters: Electron.FileFilter[]) {
    return showWebOpenDialog(filterToAccept(filters));
}

export function showWebSaveDialog(data: Uint8Array, filename: string) {
    // This function could support more flags (e.g. multiple and directory selections):
    // https://stackoverflow.com/questions/12942436/how-to-get-folder-directory-from-html-input-type-file-or-any-other-way
    const blob = new Blob([data], {type: 'application/octet-stream'});
    const anchor = document.createElement('a');
    anchor.download = filename;

    anchor.href = URL.createObjectURL(blob);
    document.body.appendChild(anchor);
    anchor.click();
    document.body.removeChild(anchor);
    URL.revokeObjectURL(anchor.href);
}

export function showNativeSaveDialog(options: Electron.SaveDialogOptions) {
    const { dialog } = electron.remote;
    return new Promise(resolve => dialog.showSaveDialog(options, resolve));
}

export async function save(data: Uint8Array, filename: string, filters: Electron.FileFilter[]) {
    if (electron) {
        if (!path.isAbsolute(filename)) {
            // If a suggested filename is given instead of a full path, show a save dialog
            const paths = await showNativeSaveDialog({ defaultPath: filename, filters });
            if (!paths) {
                return Promise.resolve();
            }
            filename = path.resolve(paths[0]);
        }
        // Save contents
        fs.writeFileSync(filename, new Buffer(data));
        return Promise.resolve(filename);
    }
    showWebSaveDialog(data, filename);
    return Promise.resolve();
}
