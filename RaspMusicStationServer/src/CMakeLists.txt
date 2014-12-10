cmake_minimum_required (VERSION 2.6)
project (RaspMusicStationServer)

# add the executable
add_executable(RaspMusicStationServer common.cpp RaspMusicStationPlayer.cpp FeiSocket.cpp RaspMusicStationServer.cpp)
target_link_libraries(RaspMusicStationServer -lpthread)

