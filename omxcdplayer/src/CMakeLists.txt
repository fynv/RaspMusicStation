cmake_minimum_required (VERSION 2.6)
project (omxcdplayer)

include_directories(/home/fei/pitools/piroot/usr/include 
/home/fei/pitools/piroot/usr/include/interface/vcos/pthreads  
/home/fei/pitools/piroot/usr/include/interface/vmcs_host/linux  
../libs/ilclient
../libs/vgfont
.)

link_directories(/home/fei/pitools/piroot/usr/lib
../libs/ilclient
../libs/vgfont
)

link_libraries(-lGLESv2 -lEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -lm -lcdio)

add_definitions(-DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DOMX -DOMX_SKIP64BIT -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM)



# add the executable
add_executable(omxcdplayer audio.c ilclient.c ilcore.c omxcdplayer.cpp main.cpp AudioCD.h AudioCD.cpp AudioCD_Play.cpp)

