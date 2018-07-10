const {app, protocol, BrowserWindow} = require('electron');
const fs = require('fs');
const path = require('path');
const url = require('url');

let mainWindow;

function createWindow() {
    const fileProtocol = 'file';
    protocol.interceptFileProtocol(fileProtocol, (request, callback) => {
        // A reference to a folder should return its index.html file
        const filePath = new url.URL(request.url).pathname;
        let resolvedPath = path.normalize(filePath);
        if (resolvedPath[0] === '\\') {
            // Remove host to pathname separator
            resolvedPath = resolvedPath.substr(1);
        }
        try {
            if (resolvedPath.slice(-1) === path.sep || fs.statSync(resolvedPath).isDirectory) {
                let index = path.posix.join(resolvedPath, 'index.html');
                if (fs.existsSync(index)) {
                    resolvedPath = index;
                }
            }
        } catch(_) {
            // Use path as is if it can't be accessed
        }
        callback({
            path: resolvedPath,
        });
    })

    mainWindow = new BrowserWindow({
        height: 600,
        width: 800,

        webPreferences: {
            webSecurity: false,
        },
    });

    let pageUrl = '';
    if (process.argv.length > 2) {
        pageUrl = process.argv[2];
    }
    if (!pageUrl.includes('://')) {
        pageUrl = url.format({
            pathname: path.join(__dirname, '../build/index.html'),
            protocol: 'file',
        });
    }
    mainWindow.loadURL(pageUrl);

    if (process.argv.includes('--dev-tools')) {
        mainWindow.webContents.openDevTools();
    }

    mainWindow.on('closed', () => {
        mainWindow = null;
    });
}

app.on('ready', createWindow);

app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') {
        app.quit();
    }
});

app.on('activate', () => {
    if (mainWindow === null) {
        createWindow();
    }
});
