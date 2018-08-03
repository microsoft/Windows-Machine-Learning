module.exports = function override(config, env) {
    // __filename and __dirname are currently mocked
    config.node = {
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
