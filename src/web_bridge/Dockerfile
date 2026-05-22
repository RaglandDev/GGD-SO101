FROM python:3.10-slim

WORKDIR /app

RUN pip install --no-cache-dir fastapi uvicorn websockets opencv-python-headless

# keep container alive for development
CMD ["tail", "-f", "/dev/null"]

