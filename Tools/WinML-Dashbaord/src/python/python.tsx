import { execFile, execFileSync } from 'child_process';
import * as fs from 'fs';
import * as https from 'https';
import * as path from 'path';
import { promisify } from 'util';
import * as yauzl from 'yauzl';

import { mkdir, winmlDataFoler } from '../persistence/appData';

const venv = mkdir(winmlDataFoler, 'venv');
export const localPython = path.join(venv, 'Scripts/python');

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
    const binaries = filterPythonBinaries(['python3', 'py', 'python']) as Array<string | null>;
    if (process.platform === 'win32') {
        binaries.push(null);  // use null to represent local, embedded version
    }
    return [...new Set(binaries)];
}

async function unzip(buffer: Buffer, directory: string) {
    return new Promise((resolve, reject) => {
        yauzl.fromBuffer(buffer, (err, zipfile) => {
            if (err) {
                return reject(err);
            }
            zipfile!.once('end', resolve);
            zipfile!.once('error', reject);
            zipfile!.on('entry', entry => {
                zipfile!.openReadStream(entry, (error, readStream) => {
                    if (error) {
                        return reject(error);
                    }
                    readStream!.once('error', reject);
                    readStream!.pipe(fs.createWriteStream(path.join(directory, entry.fileName)));
                });
            });
        });
    });
}

// Change the PTH to discover modules from Lib/site-packages, so that pip modules can be found
const pythonPth = `Lib/site-packages
python36.zip
.
# Uncomment to run site.main() automatically
import site`;

export async function downloadPython() {
    return new Promise((resolve, reject) => {
        if (process.platform !== 'win32') {
            reject('Unsupported platform');
        }
        const pythonUrl = 'https://www.python.org/ftp/python/3.7.0/python-3.7.0-embed-amd64.zip';
        const pythonFolder = mkdir(venv, 'Scripts');
        const data: Buffer[] = [];
        https.get(pythonUrl, response => {
            response
            .on('data', (x: string) => data.push(Buffer.from(x, 'binary')))
            .on('end', async () => {
                try {
                    await unzip(Buffer.concat(data), pythonFolder);
                    fs.writeFileSync(path.join(pythonFolder, 'python37._pth'), pythonPth);
                    resolve();
                } catch (err) {
                    reject(err);
                }
            })
            .on('error', reject);
        });
    });
}

export function getVenvPython() {
    for (const binary of [localPython, path.join(venv, 'bin/python')]) {
        if (fs.existsSync(binary)) {
            return binary;
        }
    }
    return;
}

export async function installPip() {
    // Python embedded distribution for Windows doesn't have pip
    return new Promise((resolve, reject) => {
        const data: Buffer[] = [];
        https.get('https://bootstrap.pypa.io/get-pip.py', response => {
            response
            .on('data', (x: string) => data.push(Buffer.from(x, 'binary')))
            .on('end', async () => {
                try {
                    const getPip = path.join(venv, 'get-pip.py');
                    fs.writeFileSync(getPip, Buffer.concat(data));
                    await python(getPip);
                    resolve();
                } catch (err) {
                    reject(err);
                }
            })
            .on('error', reject);
        });
    });
}

export async function python(...command: string[]) {
    return await promisify(execFile)(getVenvPython()!, [...command]);
}

export async function pip(...command: string[]) {
    return await python('-m', 'pip', ...command);
}

export async function installVenv(targetPython: string) {
    await promisify(execFile)(targetPython, ['-m', 'venv', venv]);
    await pip('install', '-U', 'pip');
}
