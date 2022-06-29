import {EventEmitter} from 'events';

// import { callEmit } from '../../build/Release/gloo_core_cpp.node'

import {start, stop}  from './addon'

const emitter = new EventEmitter()

emitter.on('mic', async (status) => {
    console.log('Mic Status: ', status)
})

start(emitter);

setTimeout(() => {
    stop();
    start(emitter);
    setTimeout(() => {
        stop()
    }, 5_000);
}, 50_000);
