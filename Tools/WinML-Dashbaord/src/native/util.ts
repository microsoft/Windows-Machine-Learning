import * as fs from 'fs';

export function isWeb() {
    return !fs.exists;
}
