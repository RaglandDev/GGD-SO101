from fastapi import FastAPI, WebSocket
from fastapi.responses import HTMLResponse
import uvicorn
import os

app = FastAPI()

HTML_FILE_PATH = os.path.join(os.path.dirname(__file__), "index.html")

# serve html 
@app.get("/")
async def get():
    with open(HTML_FILE_PATH, "r") as file:
        html_content = file.read()
    return HTMLResponse(html_content)

# receive compressed frames
@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    print("SUCCESS: Browser connected to Docker Backend!")
    try:
        while True:
            data = await websocket.receive_text()
            print(f"Received compressed frame: {len(data)} bytes")
    except Exception:
        print("Browser disconnected.")

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8080)
