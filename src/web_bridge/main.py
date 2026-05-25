import socket
import os
from fastapi import FastAPI, WebSocket
from fastapi.responses import HTMLResponse
from starlette.websockets import WebSocketDisconnect

app = FastAPI()

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
HTML_PATH = os.path.join(BASE_DIR, "index.html")

udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

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
    print("WebSocket link established with frontend browser.")
    
#    frame_count = 0
    try:
        while True:
            data = await websocket.receive_bytes()
#            frame_count += 1
            
            udp_socket.sendto(data, ("127.0.0.1", 9999)) # image -> Node
            
#            if frame_count % 60 == 0:
#                print(f"[Python Bridge] Forwarded {frame_count} frames. Current size: {len(data)} bytes.")
                
    except WebSocketDisconnect:
        print("Webcam client disconnected from WebSocket.")
