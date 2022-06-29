const addon = require('bindings')('gloo_core_cpp');

const start = (emitter) => {
  addon.startMicrophoneDetection(emitter.emit.bind(emitter));
}

const stop = () => {
  addon.stopMicrophoneDetection();
}

module.exports = {
  start,
  stop,
};
