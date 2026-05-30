import socket
import os
import asyncio
import json
from fastapi import FastAPI, WebSocket
from fastapi.responses import HTMLResponse
from fastapi.staticfiles import StaticFiles
from starlette.websockets import WebSocketDisconnect

app = FastAPI()
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
HTML_PATH = os.path.join(BASE_DIR, "index.html")

app.mount("/static", StaticFiles(directory=BASE_DIR), name="static")

frame_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

gaze_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
gaze_socket.bind(("0.0.0.0", 9998))
gaze_socket.setblocking(False)

gaze_clients = []

@app.get("/")
async def get():
    try:
        with open(HTML_PATH, "r") as f:
            return HTMLResponse(content=f.read(), status_code=200)
    except FileNotFoundError:
        with open("/workspace/index.html", "r") as f:
            return HTMLResponse(content=f.read(), status_code=200)

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    gaze_clients.append(websocket)
    print("WebSocket connected.")
    try:
        while True:
            data = await websocket.receive_bytes()
            frame_socket.sendto(data, ("127.0.0.1", 9999))
    except WebSocketDisconnect:
        print("WebSocket disconnected.")
    finally:
        if websocket in gaze_clients:
            gaze_clients.remove(websocket)

@app.on_event("startup")
async def start_gaze_relay():
    asyncio.create_task(gaze_relay_loop())

async def gaze_relay_loop():
    loop = asyncio.get_event_loop()
    while True:
        try:
            data = await loop.run_in_executor(None, lambda: gaze_socket.recv(1024))
            parts = data.decode("utf-8").split(",")
            if len(parts) == 11:
                msg = json.dumps({
                    "t": "gaze",
                    "lex": float(parts[0]), "ley": float(parts[1]),
                    "lax": float(parts[2]), "lay": float(parts[3]),
                    "rex": float(parts[4]), "rey": float(parts[5]),
                    "rax": float(parts[6]), "ray": float(parts[7]),
                    "pitch": float(parts[8]),
                    "yaw": float(parts[9]),
                    "roll": float(parts[10]),
                })
                for c in list(gaze_clients):
                    try:
                        await c.send_text(msg)
                    except Exception:
                        if c in gaze_clients:
                            gaze_clients.remove(c)
        except BlockingIOError:
            await asyncio.sleep(0.01)
        except Exception:
            await asyncio.sleep(0.01)
