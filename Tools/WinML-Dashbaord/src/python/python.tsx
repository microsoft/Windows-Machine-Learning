import { execFile, execFileSync } from 'child_process';
import * as fs from 'fs';
import * as https from 'https';
import * as path from 'path';
import { promisify } from 'util';
import * as yauzl from 'yauzl';

import { mkdir, winmlDataFoler } from '../persistence/appData';

const venv = path.join(winmlDataFoler, 'venv');
export const localPython = path.join(winmlDataFoler, 'python/python');

function filterPythonBinaries(binaries: string[]) {
    // Look for binaries with venv support
    return binaries.reduce((acc, x) => {
        try {
            const binary = execFileSync(x, ['-c', 'import venv; import sys; print(sys.executable)'], { encoding: 'utf8' });
            acc.push(binary.trim());
        } catch(_) {
            // Ignore binary if unavailable
        }
        return acc;
    }, [] as string[]);
}

export function getPythonBinaries() {
    const binaries = filterPythonBinaries([localPython, 'python3', 'py', 'python']) as Array<string | null>;
    if (process.platform === 'win32' && binaries[0] !== localPython) {
        binaries.push(null);  // use null to represent local, embedded version
    }
    return [...new Set(binaries)];
}

function unzip(buffer: Buffer, directory: string, success: () => void, errorCallback: (err: Error) => void) {
    yauzl.fromBuffer(buffer, (err, zipfile) => {
        if (err) {
            errorCallback(err);
            return;
        }
        zipfile!.once('end', success);
        zipfile!.once('error', errorCallback);
        zipfile!.on('entry', entry => {
            zipfile!.openReadStream(entry, (error, readStream) => {
                if (error) {
                    errorCallback(error);
                    return;
                }
                readStream!.once('error', errorCallback);
                readStream!.pipe(fs.createWriteStream(path.join(directory, entry.fileName)));
            });
        });
    });
}

export function downloadPython(success: () => void, error: (err: Error) => void) {
    if (process.platform !== 'win32') {
        throw new Error('Unsupported platform');
    }
    const pythonUrl = 'https://www.python.org/ftp/python/3.7.0/python-3.7.0-embed-amd64.zip';
    const pythonFolder = mkdir(winmlDataFoler, 'python');
    const data: Buffer[] = [];
    https.get(pythonUrl, response => {
        response
        .on('data', (x: string) => data.push(Buffer.from(x, 'binary')))
        .on('end', () => unzip(Buffer.concat(data), pythonFolder, success, error))
        .on('error', error);
    });
}

export function getVenvPython() {
    // Get Python installed in local venv
    return filterPythonBinaries([path.join(venv, 'Scripts/python'), path.join(venv, 'bin/python')])[0];
}

export async function python(...command: string[]) {
    return await promisify(execFile)(getVenvPython()!, ['-m', ...command]);
}

export async function pip(...command: string[]) {
    return await python('pip', ...command);
}

export async function installVenv(targetPython: string) {
    await promisify(execFile)(targetPython, ['-m', 'venv', venv]);
    await pip('install', '-U', 'pip');
    await pip('install', '-r', path.join(__filename, 'requirements.txt'));
}
