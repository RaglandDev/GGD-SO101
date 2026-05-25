# Gesture-and-Gaze-Directed SO-101 via Reachy Mini

## Prerequisites
- Docker Engine (Linux only) or Docker Desktop

## Usage
1. `docker compose up --build`
2. Navigate to http://localhost:8080/ws in your web browser, accept webcam permissions
3. Look in the direction of the simulated object you want the SO-101 to pick up
4. Make a fist gesture in front of your webcam in order to grab the simulated object

## Misc.
### Monitor `/human/camera/compressed` fps
1. `docker compose up --build`
2. Navigate to http://localhost:8080/ws in your web browser, accept webcam permissions
3. `docker exec -it ggd-so101-perception_processor-1 bash`
4. `source /opt/ros/humble/setup.bash`
5. `ros2 topic hz /human/camera/compressed`[^1]

[^1]: Modern browsers will de-allocate resources from non-active tabs, so you may see a large drop in frame rate if you do not have the webcam tab open.

