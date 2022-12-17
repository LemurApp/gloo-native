let Mic = require('node-microphone');
const MicMonitor = require('../gloo-native')

const callback = (status) => {
    console.log('Mic Status: ', status)
}

MicMonitor.start(callback);
console.log("STARTED");

setTimeout(() => {
    MicMonitor.stop();
    console.log("STOPPED");
}, 60 * 1_000 * 60 * 3);

// setTimeout(() => {
//     MicMonitor.start(callback);
//     console.log("STARTED");
// }, 10_000);


// setTimeout(() => {
//     MicMonitor.stop();
//     console.log("STOPPED");
// }, 25_000);
