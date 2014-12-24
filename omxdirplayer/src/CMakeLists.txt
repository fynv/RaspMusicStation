cmake_minimum_required (VERSION 2.6)
project (omxdirplayer)

# add the executable
add_executable(omxdirplayer omxdirplayer.cpp main.cpp common.cpp)
target_link_libraries(omxdirplayer -lpthread)

