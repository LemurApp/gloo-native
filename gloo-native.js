var os = require('os');
const { EventEmitter } = require('events');
const bindings = require('bindings');

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

const configureScreenTracker = (onHide, onShow) => {
  if (!loadBinding()) return;

  const emitter = new EventEmitter();
  emitter.on('hide', onHide);
  emitter.on('move', onShow);
  binding.enableWindowTracking(emitter.emit.bind(emitter));
}

const startTrackScreen = (winId) => {
  if (!loadBinding()) return;
  
  binding.startScreensharing(winId);
}

const stopTrackScreen = () => {
  if (!loadBinding()) return;
  
  binding.stopScreensharing();
}

exports.start = start;
exports.stop = stop;
exports.configureScreenTracker = configureScreenTracker;
exports.startTrackScreen = startTrackScreen;
exports.stopTrackScreen = stopTrackScreen;
