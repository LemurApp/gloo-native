declare module '@gloo/core/src/ts/addon';

import { EventEmitter } from "events";

declare function start(emitter: EventEmitter): undefined;
declare function stop(): undefined;
