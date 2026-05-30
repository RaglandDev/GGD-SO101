var video = document.getElementById('video');
var overlay = document.getElementById('overlay');
var octx = overlay.getContext('2d');
var toggleBtn = document.getElementById('toggleBtn');
var showVectors = false;

var sendCanvas = document.createElement('canvas');
const width = 320;
const height = 240;
sendCanvas.width = width;
sendCanvas.height = height;
var sendCtx = sendCanvas.getContext('2d');

var ws = new WebSocket('ws://localhost:8080/ws');
var gaze = null;
var pending = false;
var fallbackTimeout;

toggleBtn.onclick = function () {
    showVectors = !showVectors;
    toggleBtn.textContent = showVectors ? "Disable Visualization" : "Enable Visualization";
    if (!showVectors) {
        octx.clearRect(0, 0, 640, 480);
    }
};

navigator.mediaDevices.getUserMedia({ video: true })
    .then(function (s) { video.srcObject = s; });

function sendFrame() {
    if (ws.readyState === 1) {
        sendCtx.drawImage(video, 0, 0, width, height);
        sendCanvas.toBlob(function (b) {
            if (b) {
                ws.send(b);
                pending = true;
                clearTimeout(fallbackTimeout);
                fallbackTimeout = setTimeout(function () {
                    pending = false;
                    sendFrame();
                }, 150);
            }
        }, 'image/jpeg', 0.4);
    } else {
        setTimeout(sendFrame, 30);
    }
}

ws.onopen = function () {
    sendFrame();
};

ws.onmessage = function (e) {
    try {
        var m = JSON.parse(e.data);
        if (m.t === "gaze") {
            gaze = m;
            if (pending) {
                pending = false;
                clearTimeout(fallbackTimeout);
                requestAnimationFrame(sendFrame);
            }
        }
    } catch (x) { }
};

function drawArrow(x1, y1, x2, y2, color) {
    octx.beginPath();
    octx.moveTo(x1, y1);
    octx.lineTo(x2, y2);
    octx.strokeStyle = color;
    octx.lineWidth = 3;
    octx.stroke();

    var dx = x2 - x1, dy = y2 - y1;
    var len = Math.sqrt(dx * dx + dy * dy);
    if (len > 0) {
        var ux = dx / len, uy = dy / len;
        octx.beginPath();
        octx.moveTo(x2, y2);
        octx.lineTo(x2 - ux * 12 + uy * 5, y2 - uy * 12 - ux * 5);
        octx.lineTo(x2 - ux * 12 - uy * 5, y2 - uy * 12 + ux * 5);
        octx.closePath();
        octx.fillStyle = color;
        octx.fill();
    }

    octx.beginPath();
    octx.arc(x1, y1, 4, 0, 6.28);
    octx.fillStyle = '#f00';
    octx.fill();
}

(function draw() {
    if (showVectors) {
        octx.clearRect(0, 0, 640, 480);
        if (gaze) {
            var s = 640 / width;
            drawArrow(gaze.lex * s, gaze.ley * s, gaze.lax * s, gaze.lay * s, '#0f0');
            drawArrow(gaze.rex * s, gaze.rey * s, gaze.rax * s, gaze.ray * s, '#0ff');
        }
    }
    requestAnimationFrame(draw);
})();
