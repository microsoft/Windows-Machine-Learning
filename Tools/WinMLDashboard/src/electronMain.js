const {app, ipcMain, protocol, BrowserWindow} = require('electron');

const fs = require('fs');
const path = require('path');
const url = require('url');
const log = require('electron-log');

let mainWindow;
let aboutWindow;

log.transports.file.level = 'info'
log.transports.console.level = 'info'
// log.transports.file.level = 'info';
// log.transports.console.level = 'info';

if(require('electron-squirrel-startup'))
{
    return;
}

function interceptFileProtocol() {
    // Intercept the file protocol so that references to folders return its index.html file
    const fileProtocol = 'file';
    const cwd = process.cwd();
    protocol.interceptFileProtocol(fileProtocol, (request, callback) => {
        const fileUrl = new url.URL(request.url);
        const hostname = decodeURI(fileUrl.hostname);
        const filePath = decodeURI(fileUrl.pathname);
        let resolvedPath = path.normalize(filePath);
        if (resolvedPath[0] === '\\') {
            // Remove URL host to pathname separator
            resolvedPath = resolvedPath.substr(1);
        }
        if (hostname) {
            resolvedPath = path.join(hostname, resolvedPath);
            if (process.platform === 'win32') {  // File is on a share
                resolvedPath = `\\\\${resolvedPath}`;
            }
        }
        resolvedPath = path.relative(cwd, resolvedPath);
        try {
            if (fs.statSync(resolvedPath).isDirectory) {
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
    });
}

function createWindow() {
    
    log.info("=================================================")
    interceptFileProtocol();

    mainWindow = new BrowserWindow({
        height: 600,
        icon: path.join(__dirname, '../public/winml_icon.ico'),
        width: 800,
    });
    global.mainWindow = mainWindow;
    
    log.info("main window is created");
    let pageUrl;
    for (const arg of process.argv.slice(1)) {
        if (arg.includes('://')) {
            pageUrl = arg;
            break;
        }
    }
    if (pageUrl === undefined) {
        pageUrl = url.format({
            pathname: path.join(__dirname, '../build/'),
            protocol: 'file',
        });
    }
    mainWindow.loadURL(pageUrl);

    if (process.argv.includes('--dev-tools')) {
        mainWindow.webContents.openDevTools();
    }

    mainWindow.on('closed', () => {
        mainWindow = null;
        log.info("main windows is closed.")
        log.info("=================================================")
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

ipcMain.on('show-about-window', () => {
    
    openAboutWindow()
})

function openAboutWindow() {
    
  log.info("about window is opened.");
  if (aboutWindow) {
    aboutWindow.focus()
    return
  }

  aboutWindow = new BrowserWindow({
    height: 420,
    icon: path.join(__dirname, '../public/winml_icon.ico'),
    title: "About",
    width: 420,
  })

  aboutWindow.setFullScreenable(false);
  aboutWindow.setMinimizable(false);
  aboutWindow.setResizable(false);
  aboutWindow.setMenu(null);
  aboutWindow.loadURL('file://' + __dirname + '/../public/about.html');

  aboutWindow.on('closed', () => {
    log.info("about window is closed");
    aboutWindow = null
  })
}