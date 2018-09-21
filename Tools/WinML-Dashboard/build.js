var electronInstaller = require('electron-winstaller');
var path = require("path");

resultPromise = electronInstaller.createWindowsInstaller({
    appDirectory: path.join('./release/WinmlDashboard-win32-x64'), 
    authors: 'Microsoft Corporation',
    exe: 'WinmlDashboard.exe',
    outputDirectory: path.join('./installer'), 
    setupExe: 'WinmlDashBoard_setup.exe',
    setupMsi: 'WinmlDashBoard_setup.msi',
    version: "1.0.0",
  });

// tslint:disable-next-line:no-console
resultPromise.then(() => console.log("It worked!"), (e) => console.log(`No dice: ${e.message}`));