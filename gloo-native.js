var os = require('os');
const { EventEmitter } = require('events');

var binding = null;

function loadBinding() {
  if( !binding ) {
      if( ['darwin', 'win32'].includes(os.platform()) ) {
          // Linux defaults to hidraw
          binding = require('bindings')('mic_detector.node');
      }
  }
  return binding !== null;
}

const start = (cb) => {
  if (!loadBinding()) return;
  const emitter = new EventEmitter();
  emitter.on('mic', cb);
  binding.startMicrophoneDetection(emitter.emit.bind(emitter));
}

const stop = () => {
  if (!loadBinding()) return;
  
  binding.stopMicrophoneDetection();
}

exports.start = start;
exports.stop = stop;
