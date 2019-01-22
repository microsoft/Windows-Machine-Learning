/**
 * Provide dialogs, abstracting whether in web or Electron.
 */

import * as fs from 'fs';
import * as path from 'path';

import { getElectron } from './util';

export function fileFilterToAccept(filters: Electron.FileFilter[]) {
    /**
     * Convert Electron FileFilter[] to HTML accept field.
     */
    return filters.map(x => x.extensions.map(extension => `.${extension}`).join(',')).join(',');
}

export function showWebOpenDialog(accept: string) {
    const input = document.createElement('input');
    input.setAttribute('type', 'file');
    input.setAttribute('accept', accept);
    return new Promise<FileList>(resolve => {
        input.addEventListener('change', () => resolve(input.files || undefined));
        input.click();
    });
}

export function showNativeOpenDialog(options: Electron.OpenDialogOptions) {
    const { dialog } = getElectron().remote;
    return new Promise<string[]>(resolve => dialog.showOpenDialog(options, resolve));
}

export function populateFileFields(fileLikeObject: any, filePath: string) {
    fileLikeObject.lastModified = 0;
    fileLikeObject.name = path.basename(filePath);
    fileLikeObject.path = path.resolve(filePath);
    return fileLikeObject as File;
}

export function fileFromPath(filePath: string) {
    // Make the native dialogs return an object that acts like the File object returned in HTML forms.
    // This way, all functions that use dialogs can have a single code path instead of one for the web
    // and one for Electron.
    // "new File(fs.readFileSync(x), path.basename(x))" followed by "file.path = x" doesn't work because
    // the path field is read-only. Instead, we create a Blob and manually add the remaining fields (per
    // https://www.w3.org/TR/FileAPI/#dfn-file and an extra "path" field, containing the real path).
    const file = new Blob([fs.readFileSync(filePath)]);
    return populateFileFields(file, filePath);
}

export async function showOpenDialog(filters: Electron.FileFilter[]) {
    if (getElectron()) {
        const paths = await showNativeOpenDialog({ filters });
        return paths.map(fileFromPath);
    }
    return showWebOpenDialog(fileFilterToAccept(filters));
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
    const { dialog } = getElectron().remote;
    return new Promise<string>(resolve => dialog.showSaveDialog(options, resolve));
}

export async function save(data: Uint8Array, filename: string) {
    if (getElectron()) {
        // Save contents
        fs.writeFileSync(filename, new Buffer(data));
        return
    }
    showWebSaveDialog(data, filename);
    return;
}
