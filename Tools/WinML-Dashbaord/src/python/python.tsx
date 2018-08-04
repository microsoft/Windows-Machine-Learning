import { execFile, execFileSync } from 'child_process';
import * as fs from 'fs';
import * as https from 'https';
import * as path from 'path';
import { promisify } from 'util';
import * as yauzl from 'yauzl';

import { mkdir, winmlDataFoler } from '../persistence/appData';

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

function unzip(buffer: Buffer, directory: string) {
    yauzl.fromBuffer(buffer, (err, zipfile) => {
        if (err) {
            // TODO
            throw err;
        }
        zipfile!.readEntry();
        zipfile!.on('entry', entry => {
            zipfile!.openReadStream(entry, (error, readStream) => {
                if (error) {
                    // TODO
                    throw error;
                }
                readStream!.on('end', zipfile!.readEntry);
                readStream!.pipe(fs.createWriteStream(path.join(directory, entry.fileName)));
            });
        });
    });
}

export function downloadPython(success: () => void, error: () => void) {
    if (process.platform !== 'win32') {
        throw new Error('Unsupported platform');
    }
    const pythonUrl = 'https://www.python.org/ftp/python/3.7.0/python-3.7.0-embed-amd64.zip';
    const pythonFolder = mkdir(winmlDataFoler, 'python');
    const data: Buffer[] = [];
    https.get(pythonUrl, response => {
        response
        .on('data', (chunk: Buffer) => data.push(chunk))
        .on('end', () => unzip(Buffer.concat(data), pythonFolder))
        .on('error', (err: Error) => {
            // TODO
            throw err;
        });
    });
}

export function getVenvPython() {
    // Get Python installed in local venv
    const binaries = filterPythonBinaries([path.join(venv, 'Scripts/python'), path.join(venv, 'bin/python')]);
    if (binaries === []) {
        return null;
    }
    return binaries[0];
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
    await pip('install', '-r', path.join(__dirname, 'requirements.txt'));
}
