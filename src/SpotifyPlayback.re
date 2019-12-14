open ReDom;
open Spotify;

type player;

[@decco]
type disallows = {
    [@decco.default false] pausing: bool,
    [@decco.key "peeking_next"] [@decco.default false] peekingNext: bool,
    [@decco.key "peeking_prev"] [@decco.default false] peekingPrev: bool,
    [@decco.default false] resuming: bool,
    [@decco.default false] seeking: bool,
    [@decco.key "skipping_next"] [@decco.default false] skippingNext: bool,
    [@decco.key "skipping_prev"] [@decco.default false] skippingPrev: bool,
};

type repeatMode = NoRepeat | RepeatTrack | RepeatContext;

module WebPlayback = {
    [@decco] type error = { message: string };
    [@decco] type initializationError = error;
    [@decco] type authenticationError = error;
    [@decco] type accountError = error;
    [@decco] type playbackError = error;

    [@decco] type player = { [@decco.key "device_id"] deviceId: string };

    [@decco]
    type context = {
        uri: option(string),
        metadata: Js.Json.t,
    };

    [@decco]
    type trackWindow = {
        [@decco.key "current_track"] currentTrack: Js.Json.t,
        [@decco.key "previous_tracks"] previousTracks: array(Js.Json.t),
        [@decco.key "next_tracks"] nextTracks: array(Js.Json.t)
    };

    // https://developer.spotify.com/documentation/web-playback-sdk/reference/#object-web-playback-state
    // repeat_mode is incorrectly(?) documented as 1=once-repeat and 2=full-repeat
    type nonrec repeatMode = repeatMode;
    let repeatMode_encode = (v) =>
        switch v {
            | NoRepeat => 0.
            | RepeatContext => 1.
            | RepeatTrack => 2.
        }
        |> Js.Json.number;
    let repeatMode_decode = (j) =>
        switch (Js.Json.classify(j)) {
            | Js.Json.JSONNumber(0.) => Belt.Result.Ok(NoRepeat)
            | Js.Json.JSONNumber(1.) => Ok(RepeatContext)
            | Js.Json.JSONNumber(2.) => Ok(RepeatTrack)
            | _ => Decco.error("Invalid enum value", j)
        };

    [@decco]
    type state = {
        context: context,
        disallows: disallows,
        [@decco.key "track_window"] trackWindow: trackWindow,
        paused: bool,
        position: int,
        [@decco.key "repeat_mode"] repeatMode: repeatMode,
        shuffle: bool,
    };
};

module PlayerInfo = {
    [@decco]
    type actions = {
        disallows: disallows,
    };

    type nonrec repeatMode = repeatMode;
    let repeatMode_encode = (v) =>
        switch v {
            | NoRepeat => "off"
            | RepeatTrack => "track"
            | RepeatContext => "context"
        }
        |> Js.Json.string;
    let repeatMode_decode = (j) =>
        switch (Js.Json.classify(j)) {
            | Js.Json.JSONString("off") => Belt.Result.Ok(NoRepeat)
            | Js.Json.JSONString("track") => Ok(RepeatTrack)
            | Js.Json.JSONString("context") => Ok(RepeatContext)
            | _ => Decco.error("Invalid enum value", j)
        };

    [@decco]
    type t = {
        actions: actions,
        [@decco.key "progress_ms"] progressMs: option(int),
        [@decco.key "repeat_state"] repeatState: repeatMode,
        [@decco.key "shuffle_state"] shuffleState: bool,
        item: option(Spotify.Types.Track.t),
    };
};

[@bs.set] external setPlaybackSdkReadyListener : Window.t => (unit => unit) => unit = "onSpotifyWebPlaybackSDKReady";

/** Set the listener synchronously, but expose it in the API as a promise. */
let sdkInitialized = Js.Promise.make((~resolve, ~reject as _) => {
    setPlaybackSdkReadyListener(Window.window, () => {
        let u = ();
        resolve(. u);
    });
});

[@bs.scope "window.Spotify"] [@bs.new]
external makePlayer: Js.t({..}) => player = "Player";

let makePlayer = (accessToken, name) =>
    makePlayer({
        "name": name,
        "getOAuthToken": (cb) => cb(accessToken)
    });

[@bs.send] external connect : player => unit = "";
[@bs.send] external disconnect : player => unit = "";

[@bs.send.pipe: player]
external addListener : string => (Js.Json.t => unit) => unit = "addListener";

let listener = (event, decoder, cb, player) => {
    addListener(event, (res) => {
        cb(decoder(res))
    }, player);
    player;
};

let onInitializationError = listener("initialization_error", WebPlayback.error_decode);
let onAuthenticationError = listener("authentication_error", WebPlayback.error_decode);
let onAccountError = listener("account_error", WebPlayback.error_decode);
let onPlaybackError = listener("playback_error", WebPlayback.error_decode);
let onReady = listener("ready", WebPlayback.player_decode);
let onNotReady = listener("not_ready", WebPlayback.player_decode);
let onPlayerStateChanged = listener("player_state_changed", Decco.optionFromJson(WebPlayback.state_decode));

let doPlay = (~deviceId=?, ~contextUri=?, ~uris=?, ~positionMs=?, accessToken) => {
    open! Belt.Option;

    Api.(buildPut(accessToken, "/me/player/play")
    |> setOptionalQueryParam("device_id", deviceId)
    |> setOptionalParam("context_uri", contextUri |> map(_, Js.Json.string))
    |> setOptionalParam("position_ms",
        positionMs
        |> map(_, float_of_int)
        |> map(_, Js.Json.number)
    )
    |> setOptionalParam("uris",
        uris
        |> map(_, Js.Array.map(Js.Json.string))
        |> map(_, Js.Json.array)
    )
    |> Superagent.end_
    |> PromEx.map(_ => ()));
};

let play = (~deviceId=?, ~positionMs=?, accessToken) =>
    doPlay(~deviceId?, ~positionMs?, accessToken);

let playContext = (~deviceId=?, ~positionMs=?, accessToken, contextUri) =>
    doPlay(~deviceId?, ~positionMs?, ~contextUri, accessToken);

let playUris = (~deviceId=?, ~positionMs=?, accessToken, uris) =>
    doPlay(~deviceId?, ~positionMs?, ~uris, accessToken);

let pause = (~deviceId=?, accessToken) => {
    Api.(buildPut(accessToken, "/me/player/pause")
    |> setOptionalQueryParam("device_id", deviceId)
    |> Superagent.end_
    |> PromEx.map(_ => ()));
};

let seek = (~deviceId=?, accessToken, positionMs) => Superagent.(
    Api.(buildPut(accessToken, "/me/player/seek")
    |> query("position_ms", positionMs |> string_of_int)
    |> setOptionalQueryParam("device_id", deviceId)
    |> end_
    |> PromEx.map(_ => ()))
);

let getPlayerInfo = (accessToken) =>
    Api.buildGet(accessToken, "/me/player")
    |> Superagent.end_
    |> PromEx.map(({ Superagent.body }) => body)
    |> PromEx.map(Belt.Option.getExn)
    |> PromEx.map(PlayerInfo.t_decode);
