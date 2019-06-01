open Spotify.Access;

type player;
[@decco] type error = { message: string, };
[@decco] type initializationError = error;
[@decco] type authenticationError = error;
[@decco] type accountError = error;
[@decco] type playbackError = error;
[@decco] type webPlaybackPlayer = {
    device_id: string,
};
[@decco] type context = {
    uri: option(string),
    metadata: option(Js.Json.t),
};
[@decco] type disallows = {
    pausing: bool,
    peeking_next: bool,
    peeking_prev: bool,
    resuming: bool,
    seeking: bool,
    skipping_next: bool,
    skipping_prev: bool,
};
[@decco] type trackWindow = {
    current_track: Js.Json.t,
    previous_tracks: array(Js.Json.t),
    next_tracks: array(Js.Json.t),
};
[@decco] type repeatMode =
    | NoRepeat
    | RepeatOnce
    | RepeatForever;
[@decco] type webPlaybackState = {
    context: context,
    disallows: disallows,
    track_window: trackWindow,
    paused: bool,
    position: int,
    repeat_mode: repeatMode,
    shuffle: bool,
};

let sdkInitialized: Js.Promise.t(unit);

/** (token, playerName) => player */
let makePlayer:
    (token(scope(_,_,_,_,_,_,_,_,userReadPrivate,userReadEmail,userReadBirthdate,_,_,streaming,_,_,_,_)),
        string)
    => player;
let connect: player => unit;
let disconnect: player => unit;

let onInitializationError:
    (Belt.Result.t(error, Decco.decodeError) => unit, player) => player;
let onAuthenticationError:
    (Belt.Result.t(error, Decco.decodeError) => unit, player) => player;
let onAccountError:
    (Belt.Result.t(error, Decco.decodeError) => unit, player) => player;
let onPlaybackError:
    (Belt.Result.t(error, Decco.decodeError) => unit, player) => player;
let onReady:
    (Belt.Result.t(webPlaybackPlayer, Decco.decodeError) => unit,
    player) => player;
let onNotReady:
    (Belt.Result.t(webPlaybackPlayer, Decco.decodeError) => unit,
    player) => player;
let onPlayerStateChanged:
    (Belt.Result.t(option(webPlaybackState), Decco.decodeError) => unit,
    player) => player;

let play:
    (~deviceId: string=?, ~positionMs: int=?,
      token(scope(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,userModifyPlaybackState,_,_)))
    => Js.Promise.t(unit);
let playContext:
    (~deviceId: string=?, ~positionMs: int=?,
      token(scope(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,userModifyPlaybackState,_,_)),
      string)
    => Js.Promise.t(unit);
let playUris:
    (~deviceId: string=?, ~positionMs: int=?,
      token(scope(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,userModifyPlaybackState,_,_)),
      Js.Array.t(string))
    => Js.Promise.t(unit);