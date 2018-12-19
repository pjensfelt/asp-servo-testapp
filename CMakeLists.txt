cmake_minimum_required(VERSION 3.9)
project(apitestapp VERSION 0.0.1 DESCRIPTION "api test apps")

add_executable(positiontest ${CMAKE_CURRENT_SOURCE_DIR}/src/positiontest.cpp )
target_link_libraries( positiontest asp_servo )

add_executable(velocitytest ${CMAKE_CURRENT_SOURCE_DIR}/src/velocitytest.cpp )
target_link_libraries( velocitytest asp_servo )

add_executable(torquetest ${CMAKE_CURRENT_SOURCE_DIR}/src/torquetest.cpp )
target_link_libraries( torquetest asp_servo )
