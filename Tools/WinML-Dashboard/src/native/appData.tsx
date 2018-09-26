import * as fs from 'fs';
import * as path from 'path';

const nodeProcess = process;
const env = nodeProcess.env;

export const appData = env.APPDATA ||
    path.join(env.HOME || '/tmp', process.platform === 'darwin' ? 'Library/Preferences' : '.local/share');

export function mkdir(...directory: string[]) {
    const joined = path.join(...directory);
    if (fs.exists && !fs.existsSync(joined)) {  // skips if running in the web
        fs.mkdirSync(joined);
    }
    return joined;
}

export function packagedFile(...filePath: string[]) {
    // Return a path to a file packaged in the application
    return path.join(__filename, ...filePath);
}

// Point to the root if running in the web
export const winmlDataFolder = fs.exists ? mkdir(appData, 'WinmlDashBoard') : '/';
