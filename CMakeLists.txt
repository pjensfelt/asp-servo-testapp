cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(asp_servo_testapp)

add_executable(positiontest ${CMAKE_CURRENT_SOURCE_DIR}/src/positiontest.cpp )
target_link_libraries( positiontest asp_servo_api )

add_executable(velocitytest ${CMAKE_CURRENT_SOURCE_DIR}/src/velocitytest.cpp )
target_link_libraries( velocitytest asp_servo_api )

add_executable(torquetest ${CMAKE_CURRENT_SOURCE_DIR}/src/torquetest.cpp )
target_link_libraries( torquetest asp_servo_api )
