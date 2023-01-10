declare module '@lemurapp/gloo-native';

declare function start(cb: (status: boolean) => undefined): undefined;
declare function stop(): undefined;

interface Position {
    x: number;
    y: number;
    height: number;
    width: number;
}

declare function configureScreenTracker(onHide: () => undefined, onShow: (position: Position) => undefined): undefined;
declare function startTrackScreen(): undefined;
declare function stopTrackScreen(): undefined;
