let Mic = require('node-microphone');
const MicMonitor = require('../core-native')

const callback = (status) => {
    console.log('Mic Status: ', status)
}

MicMonitor.start(callback);
console.log("STARTED");

setTimeout(() => {
    MicMonitor.stop();
    console.log("STOPPED");
}, 5_000);

setTimeout(() => {
    MicMonitor.start(callback);
    console.log("STARTED");
}, 10_000);


setTimeout(() => {
    MicMonitor.stop();
    console.log("STOPPED");
}, 25_000);
