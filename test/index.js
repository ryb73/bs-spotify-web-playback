const config = require("./config.json"),
      superagent = require("superagent"),
      qs = require("qs");

let accessToken;

~function() {
    if(!location.hash) {
        let redir = encodeURIComponent(location.href);
        let scope = "streaming,user-modify-playback-state";
        location.href = `https://accounts.spotify.com/authorize?client_id=${config.clientId}&response_type=token&redirect_uri=${redir}&scope=${scope}`;
        return;
    } else {
        let q = qs.parse(location.hash.substring(1));
        accessToken = q.access_token;
        if(!accessToken) {
            console.error(q);
            throw new Error("ahh");
        }
    }
}();

window.onSpotifyWebPlaybackSDKReady = () => {
    const player = new Spotify.Player({
        name: 'bs-spotify-web-playback test client',
        getOAuthToken: cb => { cb(accessToken); }
    });

    // Error handling
    player.addListener('initialization_error', ({ message }) => { console.error("initerr", message); });
    player.addListener('authentication_error', ({ message }) => { console.error("autherr", message); });
    player.addListener('account_error', ({ message }) => { console.error("accerr", message); });
    player.addListener('playback_error', ({ message }) => { console.error("playberr", message); });

    // Playback status updates
    player.addListener('player_state_changed', state => { console.log("state", state); });

    // Ready
    player.addListener('ready', ({ device_id }) => {
        console.log('Ready with Device ID', device_id);
        showControls(device_id);
    });

    // Not Ready
    player.addListener('not_ready', ({ device_id }) => {
        console.log('Device ID has gone offline', device_id);
    });

    // Connect to the player!
    player.connect();
};

function showControls(deviceId) {
    let form = document.getElementById("play-song");
    form.addEventListener("submit", (e) => {
        e.preventDefault();

        let songId = form.querySelector("input").value;
        superagent
            .put(`https://api.spotify.com/v1/me/player/play?device_id=${deviceId}`)
            .send({ uris: [songId] })
            .set("Authorization", `Bearer ${accessToken}`)
            .then((res) => {
                console.log(res);
            })
    })

    document.getElementById("status").innerHTML = "ready";
    document.getElementById("controls").style.display = "block";
}
