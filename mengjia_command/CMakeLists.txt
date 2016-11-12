cmake_minimum_required(VERSION 2.8.3)
project(mengjia_command)

aux_source_directory(./src SRC_LIST)

#Comment this if don't want debug
set(CMAKE_BUILD_TYPE Debug)

find_package(catkin REQUIRED COMPONENTS
  drv_msgs
  dynamic_reconfigure
  geometry_msgs
  message_generation
  message_runtime
  roscpp
  rospy
  sensor_msgs
  std_msgs
)

find_package(ALSA REQUIRED)
catkin_package()
include_directories(
  ./include
  ${catkin_INCLUDE_DIRS}
)

add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} ${catkin_LIBRARIES} /usr/lib/libmsc.so /usr/lib/libjson-c.so ${ALSA_LIBRARY})

