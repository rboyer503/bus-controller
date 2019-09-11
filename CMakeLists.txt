cmake_minimum_required (VERSION 2.8)

project (bus-controller)

find_package (OpenCV REQUIRED)
include_directories (${OpenCV_INCLUDE_DIRS})
add_executable(bus-controller bus-controller.cpp SocketMgr.cpp joystick.cc JoystickMgr.cpp)  
target_link_libraries (bus-controller ${OpenCV_LIBS} -lboost_thread -lboost_system)
