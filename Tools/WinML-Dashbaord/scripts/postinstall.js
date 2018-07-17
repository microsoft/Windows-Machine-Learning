const fs = require('fs');
const path = require('path');

for (directory of ['node_modules/netron', 'node_modules/netron/src']) {
    if (!fs.existsSync(directory))
        fs.mkdirSync(directory);
}

for (source of ['view-render.css', 'view-sidebar.css', 'view.css']) {
    destination = path.join('node_modules/netron/src', source);
    if (!fs.existsSync(destination))
        fs.linkSync(path.join('static/Netron/src', source), path.join('node_modules/netron/src', source));
}
