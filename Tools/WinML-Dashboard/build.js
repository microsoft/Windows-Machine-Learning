var electronInstaller = require('electron-winstaller');
var path = require("path");

resultPromise = electronInstaller.createWindowsInstaller({
    appDirectory: path.join('./release/WinMLDashboard-win32-x64'), 
    authors: 'Microsoft Corporation',
    exe: 'WinMLDashboard.exe',
    outputDirectory: path.join('./installer'), 
    setupExe: 'WinMLDashboard_setup.exe',
    setupMsi: 'WinMLDashboard_setup.msi',
    version: "1.0.0",
  });

// tslint:disable-next-line:no-console
resultPromise.then(() => console.log("It worked!"), (e) => console.log(`No dice: ${e.message}`));