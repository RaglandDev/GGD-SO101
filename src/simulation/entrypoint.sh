#!/bin/bash
set -e

source /opt/ros/humble/setup.bash

export WEBOTS_HOME="/usr/local/webots"
export PYTHONPATH="$WEBOTS_HOME/lib/controller/python:$PYTHONPATH"
export LD_LIBRARY_PATH="$WEBOTS_HOME/lib/controller:$LD_LIBRARY_PATH"

echo "Starting Xvfb (Virtual Display)..."
export DISPLAY=:99
Xvfb :99 -screen 0 1024x768x24 -ac +extension GLX +extension RENDER +extension RANDR -noreset &
XVFB_PID=$!

# Wait for Xvfb to initialize
sleep 2

# Create a basic empty world WITHOUT textures.
# Textures often fail to load headlessly, causing black screens.
# We use basic colored geometry and a DirectionalLight instead.
WORLD_PATH="/ros2_ws/src/simulation/ggd_world.wbt"
if [ ! -f "$WORLD_PATH" ]; then
    echo "World file not found at $WORLD_PATH. Generating a default empty world..."
    mkdir -p $(dirname "$WORLD_PATH")
    cat << 'EOF' > "$WORLD_PATH"
#VRML_SIM R2023b utf8
WorldInfo {
}
Viewpoint {
  orientation -0.15 0.9 0.4 1.1
  position 2.5 1.5 2.5
}
DirectionalLight {
  direction -0.5 -1 -0.5
  intensity 2
}
Background {
  skyColor [ 0.4 0.7 1 ]
}
Solid {
  children [
    Shape {
      appearance PBRAppearance {
        baseColor 0.8 0.8 0.8
        roughness 1
        metalness 0
      }
      geometry Plane {
        size 10 10
      }
    }
  ]
}
EOF
fi

echo "Starting Webots simulation on port 1234..."

webots --batch --mode=realtime --port=1234 --stream "$WORLD_PATH"

kill $XVFB_PID
