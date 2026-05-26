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
RUN wget https://github.com/microsoft/onnxruntime/releases/download/v${ORT_VERSION}/onnxruntime-linux-x64-${ORT_VERSION}.tgz && \
    tar -zxvf onnxruntime-linux-x64-${ORT_VERSION}.tgz && \
    cp -r onnxruntime-linux-x64-${ORT_VERSION}/include/* /usr/local/include/ && \
    cp -r onnxruntime-linux-x64-${ORT_VERSION}/lib/* /usr/local/lib/ && \
    rm -rf onnxruntime-linux-x64-${ORT_VERSION}* && \
    ldconfig

WORKDIR /workspace
RUN mkdir -p src/ros2_perception/src

COPY ros2_perception/ /workspace/src/ros2_perception

RUN . /opt/ros/humble/setup.sh && \
    colcon build --packages-select  ros2_perception

COPY entrypoint.sh /workspace/entrypoint.sh
RUN chmod +x /workspace/entrypoint.sh

ENTRYPOINT ["/workspace/entrypoint.sh"]
