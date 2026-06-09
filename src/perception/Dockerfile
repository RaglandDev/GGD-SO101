FROM python:3.10-slim AS model_builder

WORKDIR /build

RUN apt-get update && apt-get install -y \
    libgl1 \
    libglib2.0-0 \
    libxcb1 \
    && rm -rf /var/lib/apt/lists/*

RUN pip install --no-cache-dir ultralytics onnx

RUN python3 -c "from ultralytics import YOLO; YOLO('yolov8n-pose.pt').export(format='onnx', imgsz=640)"

FROM ros:humble-ros-base

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    curl \
    libopencv-dev \
    ros-humble-cv-bridge \
    ros-humble-image-transport \
    ros-humble-sensor-msgs \
    && rm -rf /var/lib/apt/lists/*

ENV ORT_VERSION=1.17.1

# dynamically check the hardware architecture (uname -m) to pull x64 or aarch64
RUN ARCH=$(uname -m) && \
    if [ "$ARCH" = "x86_64" ]; then ORT_ARCH="x64"; \
    elif [ "$ARCH" = "aarch64" ]; then ORT_ARCH="aarch64"; \
    else echo "Unsupported architecture: $ARCH" && exit 1; fi && \
    wget https://github.com/microsoft/onnxruntime/releases/download/v${ORT_VERSION}/onnxruntime-linux-${ORT_ARCH}-${ORT_VERSION}.tgz && \
    tar -zxvf onnxruntime-linux-${ORT_ARCH}-${ORT_VERSION}.tgz && \
    cp -r onnxruntime-linux-${ORT_ARCH}-${ORT_VERSION}/include/* /usr/local/include/ && \
    cp -r onnxruntime-linux-${ORT_ARCH}-${ORT_VERSION}/lib/* /usr/local/lib/ && \
    rm -rf onnxruntime-linux-${ORT_ARCH}-${ORT_VERSION}* && \
    ldconfig

# copy model
RUN mkdir -p /models
COPY --from=model_builder /build/yolov8n-pose.onnx /models/yolov8n-pose.onnx

###

WORKDIR /workspace
RUN mkdir -p src/ros2_perception/src

COPY ros2_perception/ /workspace/src/ros2_perception

RUN . /opt/ros/humble/setup.sh && \
    colcon build --packages-select ros2_perception

COPY entrypoint.sh /workspace/entrypoint.sh
RUN chmod +x /workspace/entrypoint.sh

ENTRYPOINT ["/workspace/entrypoint.sh"]

