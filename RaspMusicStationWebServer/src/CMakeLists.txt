cmake_minimum_required (VERSION 2.6)
project (RaspMusicStationWebServer)

link_libraries(-lwebsockets -lpthread)

add_executable(RaspMusicStationServer common.cpp FeiSocket.cpp RaspMusicStationPlayer.cpp RaspMusicStationServer.cpp)

