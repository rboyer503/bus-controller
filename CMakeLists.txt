cmake_minimum_required (VERSION 2.8)

project (bus-controller)

find_package (OpenCV REQUIRED)
find_package (Boost REQUIRED COMPONENTS thread chrono)

include_directories (${OpenCV_INCLUDE_DIRS})
include_directories (${Boost_INCLUDE_DIRS})

add_executable (bus-controller
	bus-controller.cpp
	SocketMgr.cpp
	joystick.cc
	JoystickMgr.cpp)

target_link_libraries (bus-controller
	${OpenCV_LIBS}
	${Boost_THREAD_LIBRARY}
	${Boost_CHRONO_LIBRARY})
