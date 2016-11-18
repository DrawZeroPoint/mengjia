Mengjia is the voice system of NEU companion robot.
# Modules
### mengjia_chat
Used to communicate with the robot.
### mengjia_command
Used to send command to the robot, such as grab a bottle and send it to someone.
# Useage
- Clone this repo to your catkin_ws/src;
- Move 'mengjia' folder in mengjia_chat to /usr/lib
- Do catkin_make
- After generating the execuable, move 'dependency' folder to catkin_ws/devel/lib/mengjia_command
- Then you should good to go.