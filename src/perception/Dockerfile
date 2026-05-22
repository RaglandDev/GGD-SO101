FROM ros:humble-ros-base

WORKDIR /ros2_ws

# python, mediapipe deps 
RUN apt-get update && apt-get install -y \
    python3-pip \
    libgl1-mesa-glx \
    libglib2.0-0 \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install --no-cache-dir mediapipe opencv-python-headless

# Source ROS2 and keep container alive
CMD ["/bin/bash", "-c", "source /opt/ros/humble/setup.bash && tail -f /dev/null"]
