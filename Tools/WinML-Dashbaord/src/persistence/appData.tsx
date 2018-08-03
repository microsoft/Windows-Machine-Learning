import * as fs from 'fs';
import * as path from 'path';

export const appData = process.env.APPDATA ||
    path.join(process.env.HOME!, process.platform === 'darwin' ? 'Library/Preferences' : '.local/share');

export function mkdir(...directory: string[]) {
    const joined = path.join(...directory);
    if (!fs.existsSync(joined)) {
        fs.mkdirSync(joined);
    }
    return joined;
}

export const winmlDataFoler = mkdir(appData, 'WinML-Dashboard');
