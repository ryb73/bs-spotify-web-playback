open React;
open Belt.Result;
open Js.Promise;

type playerState = {
    player: SpotifyPlayback.player,
    name: string,
    deviceId: string
};

type state = {
    initialized: bool,
    deviceId: string,
    contextUri: string,
    uris: string,
    positionMs: string,
    playerName: string,
    player: option(playerState)
};

type action =
    | SetInitialized
    | SetPlayerName(string)
    | SetDeviceId(string)
    | SetContextUri(string)
    | SetUris(string)
    | SetPositionMs(string)
    | ClearPlayer
    | SetPlayer(SpotifyPlayback.player, string, string);

let displayError = fun
    | Ok({ SpotifyPlayback.WebPlayback.message }) => message
    | Error(e) => "couldn't decode error " ++ Belt.Option.getExn(Js.Json.stringifyAny(e));

let initialState = {
    initialized: false, deviceId: "", contextUri: "",
    uris: "spotify:track:3PK7tZzJxuoJYoik7j3p1H", positionMs: "",
    playerName: "", player: None
};

let reducer = (state, action) =>
    switch action {
        | SetInitialized => { ...state, initialized: true }
        | SetDeviceId(deviceId) => { ...state, deviceId }
        | SetContextUri(contextUri) => { ...state, contextUri }
        | SetUris(uris) => { ...state, uris }
        | SetPositionMs(positionMs) => { ...state, positionMs }
        | SetPlayerName(playerName) => { ...state, playerName }
        | ClearPlayer => { ...state, player: None }
        | SetPlayer(p, n, d) => {
            ...state, player: Some({
                player: p,
                name: n,
                deviceId: d
            })
        }
    };

let catchAndLog = (message, promise) =>
    promise
    |> catch(err => {
        Js.log2(message, err);
        resolve ();
    });

let noopSubmit = (e) => ReactEvent.Form.preventDefault(e);

[@react.component]
let make = (~token) => {
    let (state, send) = useReducer(reducer, initialState);
    let { deviceId, contextUri, uris, positionMs, playerName, player, initialized }
        = state;

    useEffect0(() => {
        SpotifyPlayback.sdkInitialized
        |> PromEx.map((_) => send(SetInitialized));
        None;
    });

    let doMakePlayer = (_) => {
        open SpotifyPlayback;

        let player = makePlayer(token, playerName);
        player
        |> onInitializationError((error) => Js.log2("init error", displayError(error)))
        |> onAuthenticationError((error) => Js.log2("auth error", displayError(error)))
        |> onAccountError((error) => Js.log2("acc error", displayError(error)))
        |> onPlaybackError((error) => Js.log2("bak error", displayError(error)))
        |> onNotReady((result) => Js.log2("not ready", result))
        |> onPlayerStateChanged(fun
            | Ok(None) => send(ClearPlayer)
            | Ok(Some(state)) => Js.log2("state changed", WebPlayback.state_encode(state))
            | error => Js.log2("state error", error)
        )
        |> onReady(fun
            | Ok({ deviceId }) => send(SetPlayer(player, playerName, deviceId))
            | error => Js.log2("ready but error", error)
        )
        |> connect;
    };

    let doPlay = (_) => {
        let deviceId = deviceId == "" ? None : Some(deviceId);
        let positionMs = positionMs == "" ? None : Some(int_of_string(positionMs));

        switch (contextUri, uris) {
            | ("", "") => SpotifyPlayback.play(~deviceId?, ~positionMs?, token)
            | (context, "") => SpotifyPlayback.playContext(~deviceId?, ~positionMs?, token, context)
            | (_, uris) => {
                let uris = Js.String.split(",", uris);
                SpotifyPlayback.playUris(~deviceId?, ~positionMs?, token, uris);
            }
        }
        |> ignore;
    };

    let doPause = (_) => {
        let deviceId = deviceId == "" ? None : Some(deviceId);
        SpotifyPlayback.pause(~deviceId?, token)
        |> catchAndLog("pause error")
        |> ignore;
    };

    let deviceIdChanged = (e) =>
        send(SetDeviceId(ReactEvent.Form.currentTarget(e)##value));

    let contextUriChanged = (e) =>
        send(SetContextUri(ReactEvent.Form.currentTarget(e)##value));

    let urisChanged = (e) =>
        send(SetUris(ReactEvent.Form.currentTarget(e)##value));

    let positionMsChanged = (e) =>
        send(SetPositionMs(ReactEvent.Form.currentTarget(e)##value));

    let playerNameChanged = (e) =>
        send(SetPlayerName(ReactEvent.Form.currentTarget(e)##value));

    let disconnectPlayer = (_) => {
        let player = Belt.Option.getExn(player).player;
        SpotifyPlayback.disconnect(player);
    };

    let activePlayer =
        switch player {
            | Some({ name, deviceId }) =>
                <div>
                    <div>(string("Active player: " ++ name))</div>
                    <div>(string("Device ID: " ++ deviceId))</div>
                    <div>
                        <button onClick=disconnectPlayer>(string("Disconnect"))</button>
                    </div>
                </div>
            | None => ReasonReact.null
        };

    let getPlayer = (_) =>
        SpotifyPlayback.getPlayerInfo(token)
        |> PromEx.map(Js.log2("player info"))
        |> catchAndLog("player info err")
        |> ignore;

    initialized ?
        <form className="col card" onSubmit=noopSubmit>
            <h2>(string("Playback"))</h2>

            <div>
                <button onClick=doMakePlayer>(string("MakePlayer"))</button>

                <input type_="text" value=playerName className=Css.(style([width(px(125))]))
                    placeholder="player name" onChange=playerNameChanged />
            </div>

            <div>
                <button onClick=doPlay>(string("Play"))</button>

                <input type_="text" value=deviceId placeholder="deviceId"
                    onChange=deviceIdChanged />
                <input type_="text" value=contextUri placeholder="contextUri"
                    onChange=contextUriChanged />
                <input type_="text" value=uris placeholder="uris"
                    onChange=urisChanged />
                <input type_="text" value=positionMs placeholder="positionMs"
                    onChange=positionMsChanged />
            </div>

            <div>
                <button onClick=doPause>(string("Pause"))</button>

                <input type_="text" value=deviceId placeholder="deviceId"
                    onChange=deviceIdChanged />
            </div>

            <div>
                <button onClick=getPlayer>(string("Get Player Info"))</button>
            </div>

            (activePlayer)
        </form>
    :
        <div>(string("initializing"))</div>;
};
