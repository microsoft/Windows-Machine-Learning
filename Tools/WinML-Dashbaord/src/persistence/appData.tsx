import * as fs from 'fs';
import * as path from 'path';

export const appData = process.env.APPDATA ||
    path.join(process.env.HOME || '/tmp', process.platform === 'darwin' ? 'Library/Preferences' : '.local/share');

export function mkdir(...directory: string[]) {
    const joined = path.join(...directory);
    if (fs.exists && !fs.existsSync(joined)) {  // skips if running in the web
        fs.mkdirSync(joined);
    }
    return joined;
}

// Point to the root if running in the web
export const winmlDataFoler = fs.exists ? mkdir(appData, 'WinML-Dashboard') : '/';
