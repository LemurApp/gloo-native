let Mic = require('node-microphone');
const MicMonitor = require('../core-native')

const callback = (status) => {
    console.log('Mic Status: ', status)
}
let mic = new Mic();
mic.on('info', (info) => {
	console.log("MIC INFO:", info);
});
mic.on('error', (error) => {
	console.error("MIC ERR:", error);
});

MicMonitor.start(callback);

setTimeout(() => {
    let micStream = mic.startRecording();
}, 5_000);
setTimeout(() => {
    mic.stopRecording();
}, 10_000);

setTimeout(() => {
    MicMonitor.stop();
    MicMonitor.start(callback);
    setTimeout(() => {
        MicMonitor.stop()
    }, 5_000);
}, 50_000);
