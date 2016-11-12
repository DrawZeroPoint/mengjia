cmake_minimum_required(VERSION 2.8.3)
project(mengjia_chat)

set(CMAKE_BUILD_TYPE Debug)

find_package(catkin REQUIRED COMPONENTS dynamic_reconfigure roscpp rospy std_msgs)

find_package(ALSA REQUIRED)

catkin_package()

include_directories(${catkin_INCLUDE_DIRS})

add_executable(mengjia_chat src/mengjia_chat.cpp)
target_link_libraries(mengjia_chat ${catkin_LIBRARIES} /usr/lib/libmsc.so /usr/lib/libjson-c.so /usr/lib/x86_64-linux-gnu/libasound.so)
