Mengjia is the voice system of NEU companion robot.
# Modules
### mengjia_chat
Used to communicate with the robot.
### mengjia_command
Used to send command to the robot, such as grab a bottle and send it to someone.
# Usage
- Clone this repo to your catkin_ws/src;
- Copy 'mengjia' folder in mengjia_chat to /usr/lib
- Do catkin_make
- After generating the executable, Copy the contents of 'dependency' folder in 'mengjia_command' to catkin_ws/devel/lib/mengjia_command, and put the contents together with the executable.
- Then you are good to go.
