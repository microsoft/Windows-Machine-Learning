import { execFile, ExecFileOptions, execFileSync } from 'child_process';
import * as fs from 'fs';
import * as https from 'https';
import * as path from 'path';
import { promisify } from 'util';
import * as yauzl from 'yauzl';

import { mkdir, winmlDataFolder } from '../persistence/appData';

const localPython = mkdir(winmlDataFolder, 'python');
const embeddedPythonBinary = path.join(localPython, 'python.exe');

function filterPythonBinaries(binaries: string[]) {
    // Look for binaries with venv support
    return [...new Set(binaries.reduce((acc, x) => {
        try {
            if (x === embeddedPythonBinary && fs.existsSync(x)) {  // Install directly (without a venv) in embedded Python installations
                acc.push(x);
            } else {
                const binary = execFileSync(x, ['-c', 'import venv; import sys; print(sys.executable)'], { encoding: 'utf8' });
                acc.push(binary.trim());
            }
        } catch(_) {
            // Ignore binary if unavailable
        }
        return acc;
    }, [] as string[]))];
}

export function getPythonBinaries() {
    const binaries = filterPythonBinaries(['python3', 'py', 'python']) as Array<string | null>;
    if (process.platform === 'win32') {
        binaries.push(null);  // use null to represent local, embedded version
    }
    return binaries;
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
                if (entry.fileName.endsWith('/')) {
                    try {
                        return fs.mkdirSync(path.join(directory, entry.fileName));
                    } catch (e) {
                        return reject(e);
                    }
                }
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

async function downloadBinaryFile(url: string) {
    return new Promise((resolve, reject) => {
        const data: Buffer[] = [];
        https.get(url, response => {
            response
            .on('data', (x: string) => data.push(Buffer.from(x, 'binary')))
            .on('end', async () => {
                resolve(Buffer.concat(data));
            })
            .on('error', reject);
        });
    });

}

// Change the PTH to discover modules from Lib/site-packages, so that pip modules can be found
const pythonPth = `Lib/site-packages
python37.zip
.
# Uncomment to run site.main() automatically
import site`;

export async function downloadPython() {
    if (process.platform !== 'win32') {
        throw Error('Unsupported platform');
    }
    return new Promise(async (resolve, reject) => {
        try {
            const data = await downloadBinaryFile('https://www.python.org/ftp/python/3.7.0/python-3.7.0-embed-amd64.zip') as Buffer;
            await unzip(data, localPython);
            fs.writeFileSync(path.join(localPython, 'python37._pth'), pythonPth);
        } catch (err) {
            reject(err);
        }
        resolve();
    });
}

export async function downloadProtobuf() {
    if (process.platform !== 'win32') {
        throw Error('Unsupported platform');
    }
    return new Promise(async (resolve, reject) => {
        try {
            const data = await downloadBinaryFile('https://f001.backblazeb2.com/file/ticast/protobuf.zip') as Buffer;
            await unzip(data, winmlDataFolder);
        } catch (err) {
            return reject(err);
        }
        resolve();
    });
}

export async function downloadPip() {
    // Python embedded distribution for Windows doesn't have pip
    return new Promise(async (resolve, reject) => {
        const installer = path.join(localPython, 'get-pip.py');
        try {
            const data = await downloadBinaryFile('https://bootstrap.pypa.io/get-pip.py') as Buffer;
            fs.writeFileSync(installer, data);
            await python(installer);
        } catch (err) {
            return reject(err);
        }
        resolve();
    });
}

export function getLocalPython() {  // Get the local (embedded or venv) Python
    return filterPythonBinaries([embeddedPythonBinary, path.join(localPython, 'Scripts/python'), path.join(localPython, 'bin/python')])[0];
}

async function execFilePromise(file: string, args: string[], options?: ExecFileOptions) {
    const run = async () => await promisify(execFile)(file, args, options);
    for (let i = 0; i < 10; i++) {
        try {
            return await run();
        } catch (err) {
            if (err.code !== 'ENOENT') {
                throw err;
            }
            // tslint:disable-next-line:no-console
            console.warn('Ignoring possibly spurious ENOENT in child process invocation');
        }
    }
    return await run();
}

export async function python(...command: string[]) {
    const binary = getLocalPython();
    if (!binary) {
        throw Error('Failed to find local Python');
    }
    const nodeProcess = process;
    const PATH = `${nodeProcess.env.PATH}${path.delimiter}${path.join(winmlDataFolder, 'tools', 'protobuf')}`;
    const env = {
        ...nodeProcess.env,
        CMAKE_INCLUDE_PATH: path.join(winmlDataFolder, 'include'),
        CMAKE_LIBRARY_PATH: path.join(winmlDataFolder, 'bin'),
        PATH,
        Path: PATH,
    };
    return await execFilePromise(binary, command, { env });
}

export async function pip(...command: string[]) {
    return await python('-m', 'pip', ...command);
}

export async function installVenv(targetPython: string) {
    await promisify(execFile)(targetPython, ['-m', 'venv', localPython]);
    await pip('install', '-U', 'pip');
}
