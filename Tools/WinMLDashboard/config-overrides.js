module.exports = function override(config, env) {
    config.node = {
        __dirname: false,
        __filename: false,

        Buffer: false,
        child_process: false,
        dgram: false,
        fs: false,
        global: false,
        net: false,
        process: false,
        setImmediate: false,
        tls: false,
    };
    config.target = 'electron-renderer';
    return config;
}
