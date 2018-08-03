import { execFile, execFileSync } from 'child_process';
import * as https from 'https';
import * as path from 'path';
import * as unzipper from 'unzipper';
import * as util from 'util';

import { mkdir, winmlDataFoler } from '../persistence/appData';

const execFilePromisse = util.promisify(execFile);
const venv = path.join(winmlDataFoler, 'venv');

function filterPythonBinaries(binaries: string[]) {
    // Look for binaries with venv support
    return binaries.filter((binary) => {
        try {
            execFileSync(binary, ['-c', 'import venv']);
            return true;
        } catch(_) {
            return;
        }
    });
}

export function getPythonBinaries() {
    const localPython = path.join(winmlDataFoler, 'python/python');
    const binaries = filterPythonBinaries([localPython, 'python3', 'py', 'python']) as Array<string | null>;
    if (process.platform === 'win32' && binaries[0] !== localPython) {
        binaries.push(null);  // use null to represent local, embedded version
    }
    return binaries;
}

function downloadPython(success: () => void, error: () => void) {
    if (process.platform !== 'win32') {
        throw new Error('Unsupported platform');
    }
    const pythonUrl = 'https://www.python.org/ftp/python/3.7.0/python-3.7.0-embed-amd64.zip';
    const pythonFolder = mkdir(winmlDataFoler, 'python');
    https.get(pythonUrl, response =>
        response.pipe(unzipper.Extract({ path: pythonFolder }))
        .on('finish', success)
        .on('error', error));
}

export async function python(...command: string[]) {
    const binary = filterPythonBinaries([path.join(venv, 'Scripts/python'), path.join(venv, 'bin/python')])[0];
    return await execFilePromisse(binary, ['-m', ...command]);
}

export async function pip(...command: string[]) {
    return await python('pip', ...command);
}

async function installVenv(targetPython: string) {
    await execFilePromisse(targetPython, ['-m', 'venv', venv]);
    await pip('install', '-U', 'pip');
    await pip('install', '-r', path.join(__dirname, 'requirements.txt'));
}

module.exports = {
    downloadPython,
    getPythonBinaries,
    installVenv,
};
