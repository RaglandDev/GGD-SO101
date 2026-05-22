FROM ros:humble-ros-base

WORKDIR /ros2_ws

# system utils for latency monitoring
RUN apt-get update && apt-get install -y \
    iputils-ping \
    net-tools \
    && rm -rf /var/lib/apt/lists/*

CMD ["/bin/bash", "-c", "source /opt/ros/humble/setup.bash && tail -f /dev/null"]
