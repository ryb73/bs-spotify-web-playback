open Spotify;
open Access;

type player;

[@decco] type disallows = {
    pausing: bool,
    peekingNext: bool,
    peekingPrev: bool,
    resuming: bool,
    seeking: bool,
    skippingNext: bool,
    skippingPrev: bool,
};

type repeatMode = NoRepeat | RepeatTrack | RepeatContext;

module WebPlayback: {
    [@decco] type error = { message: string, };
    [@decco] type initializationError = error;
    [@decco] type authenticationError = error;
    [@decco] type accountError = error;
    [@decco] type playbackError = error;
    [@decco] type player = {
        deviceId: string,
    };
    [@decco] type context = {
        uri: option(string),
        metadata: Js.Json.t,
    };
    [@decco] type trackWindow = {
        currentTrack: Js.Json.t,
        previousTracks: array(Js.Json.t),
        nextTracks: array(Js.Json.t),
    };
    [@decco] type nonrec repeatMode = repeatMode;
    [@decco] type state = {
        context: context,
        disallows: disallows,
        trackWindow: trackWindow,
        paused: bool,
        position: int,
        repeatMode: repeatMode,
        shuffle: bool,
    };
};

module PlayerInfo: {
    [@decco]
    type actions = {
        disallows: disallows,
    };

    [@decco] type nonrec repeatMode = repeatMode;

    [@decco]
    type t = {
        actions: actions,
        [@decco.key "progress_ms"] progressMs: option(int),
        [@decco.key "repeat_state"] repeatState: repeatMode,
        [@decco.key "shuffle_state"] shuffleState: bool,
        item: option(Types.Track.t),
    };
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
    (Belt.Result.t(WebPlayback.error, Decco.decodeError) => unit, player) => player;
let onAuthenticationError:
    (Belt.Result.t(WebPlayback.error, Decco.decodeError) => unit, player) => player;
let onAccountError:
    (Belt.Result.t(WebPlayback.error, Decco.decodeError) => unit, player) => player;
let onPlaybackError:
    (Belt.Result.t(WebPlayback.error, Decco.decodeError) => unit, player) => player;
let onReady:
    (Belt.Result.t(WebPlayback.player, Decco.decodeError) => unit,
    player) => player;
let onNotReady:
    (Belt.Result.t(WebPlayback.player, Decco.decodeError) => unit,
    player) => player;
let onPlayerStateChanged:
    (Belt.Result.t(option(WebPlayback.state), Decco.decodeError) => unit,
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
let pause:
    (~deviceId: string=?,
      token(scope(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,userModifyPlaybackState,_,_)))
    => Js.Promise.t(unit);

let getPlayerInfo:
    token(scope(_,_,_,_,_,_,_,_,_,_,_,userReadPlaybackState,_,_,_,_,_,_))
    => Js.Promise.t(Belt.Result.t(PlayerInfo.t, Decco.decodeError));
