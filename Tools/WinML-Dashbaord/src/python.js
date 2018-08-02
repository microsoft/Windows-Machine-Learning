const { execFile, execFileSync } = require('child_process');
const fs = require('fs');
const https = require('https');
const path = require('path');
const unzipper = require('unzipper');
const util = require('util');

const appData = process.env.APPDATA ||
    path.join(process.env.HOME, process.platform == 'darwin' ? 'Library/Preferences' : '.local/share');

function mkdir(...directory) {
    directory = path.join(...directory);
    if (!fs.existsSync(directory)) {
        fs.mkdirSync(directory);
    }
    return directory;
}

function filterPythonBinaries(binaries) {
    // Look for binaries with venv support
    return binaries.filter((binary) => {
        try {
            execFileSync(binary, ['-c', 'import venv']);
            return true;
        } catch(_) {}
    }, []);
}

function getPythonBinaries() {
    const localPython = path.join(appData, 'WinML-Dashboard/python/python')
    const binaries = filterPythonBinaries([localPython, 'python3', 'py', 'python'])
    if (process.platform === 'win32' && binaries[0] !== localPython) {
        binaries.push(null);  // use null to represent local, embedded version
    }
    return binaries;
}

function downloadPython(success, error) {
    if (process.platform !== 'win32') {
        throw new Error('Unsupported platform');
    }
    const pythonUrl = 'https://www.python.org/ftp/python/3.7.0/python-3.7.0-embed-amd64.zip';
    const appFolder = mkdir(appData, 'WinML-Dashboard');
    const pythonFolder = mkdir(appFolder, 'python');
    https.get(pythonUrl, response =>
        response.pipe(unzipper.Extract({ path: pythonFolder }))
        .on('finish', success)
        .on('error', error));
}

async function installVenv(targetPython) {
    const execFilePromisse = util.promisify(execFile);
    const venv = path.join(mkdir(appData, 'WinML-Dashboard'), 'venv');

    await execFilePromisse(targetPython, ['-m', 'venv', venv]);
    const python = filterPythonBinaries([path.join(venv, 'bin/python'), path.join(venv, 'Scripts/python')])[0];
    async function pip(...command) {
        return await execFilePromisse(python, ['-m', 'pip', ...command]);
    }
    await pip('install', '-U', 'pip')
    await pip('install', '-r', path.join(__dirname, 'requirements.txt'))
}

module.exports = {
    downloadPython,
    getPythonBinaries,
    installVenv,
};
