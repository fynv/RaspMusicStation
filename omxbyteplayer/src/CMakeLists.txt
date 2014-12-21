cmake_minimum_required (VERSION 2.6)
project (omxbyteplayer)

include_directories(/usr/include 
/opt/vc/include
/opt/vc/include/interface/vcos/pthreads  
/opt/vc/include/interface/vmcs_host/linux  
../libs/ilclient
../libs/vgfont
.)

link_directories(/usr/lib
/opt/vc/lib
../libs/ilclient
../libs/vgfont
)

link_libraries(-lGLESv2 -lEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -lm)

add_definitions(-DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DOMX -DOMX_SKIP64BIT -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM)



# add the executable
add_executable(omxbyteplayer audio.c ilclient.c ilcore.c main.cpp playbuffer.cpp)

