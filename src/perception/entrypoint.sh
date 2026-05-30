#!/bin/bash
set -e

source /opt/ros/humble/setup.bash
source /workspace/install/setup.bash

cd /workspace

ros2 run ros2_perception perception_node
