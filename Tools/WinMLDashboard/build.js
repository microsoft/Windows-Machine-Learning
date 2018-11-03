var electronInstaller = require('electron-winstaller');
var path = require('path');

resultPromise = electronInstaller.createWindowsInstaller({
    appDirectory: path.join('./release/WinMLDashboard-win32-x64'), 
    authors: 'Microsoft Corporation',
    exe: 'WinMLDashboard.exe',
    iconUrl: 'https://github.com/Microsoft/Windows-Machine-Learning/blob/RS5/Tools/WinMLDashboard/public/winml_icon.ico',
    noMsi: true,
    outputDirectory: path.join('./installer'), 
    setupExe: 'WinMLDashboard_setup_0.5.1.exe',
    setupIcon: './public/winml_icon.ico',
    version: '0.5.1',
  });

// tslint:disable-next-line:no-console
resultPromise.then(() => console.log('It worked!'), (e) => console.log(`No dice: ${e.message}`)); 
