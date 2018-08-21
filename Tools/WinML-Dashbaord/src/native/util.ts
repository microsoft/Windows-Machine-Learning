import * as fs from 'fs';

import { createMenu } from './menu';

export function isWeb() {
    return !fs.exists || typeof it === 'function';
}

let electron: typeof Electron;
if (!isWeb()) {
    import('electron')
        .then(mod => { electron = mod; createMenu(electron); })
        // tslint:disable-next-line:no-console
        .catch(console.error);
}

export function getElectron() {
    return electron;
}
