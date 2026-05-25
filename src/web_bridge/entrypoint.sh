#!/bin/bash
set -e

source /opt/ros/humble/setup.bash
source /workspace/install/setup.bash

ros2 run ros2_bridge image_bridge_node &

cd /workspace
exec uvicorn main:app --host 0.0.0.0 --port 8080
