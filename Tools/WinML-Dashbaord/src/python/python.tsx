import { execFile, ExecFileOptions, execFileSync } from 'child_process';
import * as fs from 'fs';
import * as https from 'https';
import * as os from 'os';
import * as path from 'path';
import * as yauzl from 'yauzl';

import { mkdir, winmlDataFolder } from '../persistence/appData';

const localPython = mkdir(winmlDataFolder, 'python');
const embeddedPythonBinary = path.join(localPython, 'python.exe');

function filterPythonBinaries(binaries: string[]) {
    return [...new Set(binaries.reduce((acc, x) => {
        if (x === embeddedPythonBinary && fs.existsSync(x)) {  // Install directly (without a venv) in embedded Python installations
            acc.push(x);
        } else {
            try {
                const binary = execFileSync(x, ['-c', 'import venv; import sys; print(sys.executable)'], { encoding: 'utf8' });
                acc.push(binary.trim());
            } catch(_) {
                // Ignore binary if unavailable or no venv support
            }
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
                const destination = path.join(directory, entry.fileName);
                if (entry.fileName.endsWith('/')) {
                    if (fs.existsSync(destination)) {
                        return;
                    }
                    try {
                        return fs.mkdirSync(destination);
                    } catch (e) {
                        return reject(e);
                    }
                }
                zipfile!.openReadStream(entry, (error, readStream) => {
                    if (error) {
                        return reject(error);
                    }
                    readStream!.once('error', reject);
                    readStream!.pipe(fs.createWriteStream(destination));
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
python36.zip
.
# Uncomment to run site.main() automatically
import site`;

export async function downloadPython() {
    if (process.platform !== 'win32') {
        throw Error('Unsupported platform');
    }
    return new Promise(async (resolve, reject) => {
        try {
            const data = await downloadBinaryFile('https://www.python.org/ftp/python/3.6.6/python-3.6.6-embed-amd64.zip') as Buffer;
            await unzip(data, localPython);
            fs.writeFileSync(path.join(localPython, 'python36._pth'), pythonPth);
            const includes = await downloadBinaryFile('https://f001.backblazeb2.com/file/ticast/python37_include.zip') as Buffer;
            await unzip(includes, localPython);
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
            await python([installer]);
        } catch (err) {
            return reject(err);
        }
        resolve();
    });
}

export function getLocalPython() {  // Get the local (embedded or venv) Python
    return filterPythonBinaries([embeddedPythonBinary, path.join(localPython, 'Scripts/python'), path.join(localPython, 'bin/python')])[0];
}

interface IOutputListener {
    stdout: (output: string) => void,
    stderr: (output: string) => void,
}

async function execFilePromise(file: string, args: string[], options?: ExecFileOptions, listener?: IOutputListener) {
    const run = async () => new Promise((resolve, reject) => {
        const childProcess = execFile(file, args, {...options});
        if (listener) {
            childProcess.stdout.on('data', listener.stdout);
            childProcess.stderr.on('data', listener.stderr);
        }
        childProcess.on('exit', (code, signal) => {
            if (code !== 0) {
                return reject(Error(`Child process ${file} ${code !== null ? `exited with code ${code}` : `killed by signal ${signal}`}`));
            }
            resolve();
        });
    });
    for (let i = 0; i < 10; i++) {
        try {
            return await run();
        } catch (err) {
            if (err.code !== 'ENOENT') {  // ignoring possible spurious ENOENT in child process invocation
                throw err;
            }
        }
    }
    return run();
}

export async function python(command: string[], options?: ExecFileOptions, listener?: IOutputListener) {
    const binary = getLocalPython();
    if (!binary) {
        throw Error('Failed to find local Python');
    }
    return execFilePromise(binary, command, options, listener);
}

export async function pip(command: string[], listener?: IOutputListener) {
    let options;
    if (getLocalPython() === embeddedPythonBinary) {
        const nodeProcess = process;
        const PATH = [nodeProcess.env.PATH, path.join(winmlDataFolder, 'tools', 'protobuf')].join(path.delimiter);
        options = {
            cwd: os.tmpdir(),
            env: {
                ...nodeProcess.env,
                CMAKE_INCLUDE_PATH: path.join(winmlDataFolder, 'include'),
                CMAKE_LIBRARY_PATH: path.join(winmlDataFolder, 'lib'),
                // See https://github.com/google/protobuf/blob/master/cmake/README.md#notes-on-compiler-warnings
                CXXFLAGS: '/wd4251',
                PATH,
                PYTHON_INCLUDE_DIR: path.join(winmlDataFolder, 'python', 'include'),
                PYTHON_LIBRARY: path.join(winmlDataFolder, 'python', 'python36.dll'),
                Path: PATH,
            },
        };
    }
    return python(['-m', 'pip', ...command], options, listener);
}

export async function installVenv(targetPython: string, listener?: IOutputListener) {
    await execFilePromise(targetPython, ['-m', 'venv', localPython], {}, listener);
    await pip(['install', '-U', 'pip'], listener);
}
