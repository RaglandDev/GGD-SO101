FROM ros:humble-ros-base

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install fastapi uvicorn websockets

WORKDIR /workspace
RUN mkdir -p src/ros2_bridge/src

COPY main.py index.html /workspace/
COPY ros2_bridge/ /workspace/src/ros2_bridge/

RUN . /opt/ros/humble/setup.sh && \
    colcon build --packages-select ros2_bridge

COPY entrypoint.sh /workspace/entrypoint.sh

ENTRYPOINT ["/workspace/entrypoint.sh"]
