graph TD
    %% Base Styling Definitions
    classDef hostStyle fill:#2c3e50,stroke:#34495e,stroke-width:2px,color:#fff;
    classDef containerStyle fill:#16a085,stroke:#1abc9c,stroke-width:2px,color:#fff;
    classDef topicStyle fill:#2980b9,stroke:#3498db,stroke-width:2px,color:#fff;
    classDef socketStyle fill:#d35400,stroke:#e67e22,stroke-width:2px,color:#fff;

    %% --- PHYSICAL HOST ENVIRONMENT ---
    subgraph Host_Laptop [Any Host Hardware: macOS, Windows, or Linux]
        Operator[Human Operator<br>Gaze & Gestures]
        Cam[Built-in Webcam or USB Camera]
        Browser[Native Web Browser<br>http://localhost:8080]
        Display[Host Monitor]
        
        Operator -->|Interacts With| Browser
        Browser -->|Natively Captures| Cam
        Display -->|Visual Feedback| Operator
    end
    class Host_Laptop hostStyle;

    %% --- HOST TO DOCKER NETWORKING ---
    Cam -->|WebSocket Binary Stream<br>JPEG Compressed Strings| Port_8080((Port 8080))
    Port_5000((Port 5000)) -->|MJPEG Video Stream<br>JPEG Compressed Frames| Display

    %% --- PORTABLE DOCKER CONTAINERS ---
    subgraph Docker_Compose_Stack [Portable Linux Docker Environment]
        
        %% Container 1
        subgraph C1 [Container 1: web_input_bridge]
            Port_8080 --> WebBridge[FastAPI / WebSocket Server]
            WebBridge -->|Publish| Topic_CamIn[/human/camera/compressed<br>sensor_msgs/CompressedImage/]
        end
        class C1 containerStyle;

        %% Container 2
        subgraph C2 [Container 2: perception_processor]
            Topic_CamIn --> MP_Node[human_tracker_node<br>MediaPipe Face Mesh & Hands]
            MP_Node -->|Decompresses JPEG internally<br>Extracts Head Pose & Gestures| MP_Node
            MP_Node -->|Publish| Topic_Neck[/reachy/neck_cmd<br>geometry_msgs/Vector3/]
            MP_Node -->|Publish| Topic_Gest[/human/gesture_cmd<br>std_msgs/String/]
        end
        class C2 containerStyle;

        %% Container 3
        subgraph C3 [Container 3: simulation_control]
            Topic_Neck --> ReachyHead[Reachy Mini Lite Head<br>Simulated First-Person Avatar]
            Topic_Gest --> SharedAut[shared_autonomy_node<br>State Machine & Logic]
            
            ReachyHead -->|Virtual Camera Engine| Topic_ReachyCam[/reachy/camera/compressed<br>sensor_msgs/CompressedImage/]
            Topic_ReachyCam --> SharedAut
            Topic_ReachyCam --> Port_5000
            
            SharedAut -->|Calculated IK Target| MoveIt[manipulator_control_node<br>MoveIt2 Control Stack]
            MoveIt --> Arm[SO-101 Arm Actuators<br>Simulated Manipulation Actor]
            
            Arm -->|Telemetry Feedback| Topic_Joints[/joint_states<br>sensor_msgs/JointState/]
        end
        class C3 containerStyle;

        %% Container 4
        subgraph C4 [Container 4: triage_supervisor]
            Topic_CamIn -.->|Monitor Input Latency| Watchdog[triage_supervisor_node<br>Lifecycle Health Watchdog]
            Topic_Joints -.->|Monitor Arm Stalls/Spikes| Watchdog
            
            Watchdog -.->|Broadcast Health Logs| Topic_Logs[/sys/triage_status<br>std_msgs/String/]
            Watchdog ==>|Emergency Override Line:<br>Trigger Safe Halt on Fault| Arm
        end
        class C4 containerStyle;

    end
    class Docker_Compose_Stack hostStyle;

    %% Apply formatting to core communication links
    class Topic_CamIn,Topic_Neck,Topic_Gest,Topic_ReachyCam,Topic_Joints,Topic_Logs topicStyle;
    class Port_8080,Port_5000 socketStyle;
