let Mic = require('node-microphone');
const MicMonitor = require('../gloo-native')

const callback = (status) => {
    console.log('Mic Status: ', status)
}

MicMonitor.configureScreenTracker(() => {
    console.log("Hiding window:");
}, (pos) => {
    console.log("Move window: ", pos);
});


MicMonitor.start(callback);
console.log("STARTED");
MicMonitor.startTrackScreen(8761);

setTimeout(() => {
    MicMonitor.stop();
    console.log("STOPPED");
MicMonitor.stopTrackScreen();
}, 5_000);

setTimeout(() => {
    MicMonitor.start(callback);
    console.log("STARTED");
}, 10_000);


setTimeout(() => {
    MicMonitor.stop();
    console.log("STOPPED");
}, 25_000);
