FROM ros:humble-ros-base

WORKDIR /ros2_ws

# webots, tf2 
RUN apt-get update && apt-get install -y \
    ros-humble-webots-ros2 \
    ros-humble-tf2-ros \
    ros-humble-geometry-msgs \
    && rm -rf /var/lib/apt/lists/*

CMD ["/bin/bash", "-c", "source /opt/ros/humble/setup.bash && tail -f /dev/null"]
