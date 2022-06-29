const MicMonitor = require('../core-native')

const callback = (status) => {
    console.log('Mic Status: ', status)
}

MicMonitor.start(callback);

setTimeout(() => {
    MicMonitor.stop();
    MicMonitor.start(callback);
    setTimeout(() => {
        MicMonitor.stop()
    }, 5_000);
}, 50_000);
