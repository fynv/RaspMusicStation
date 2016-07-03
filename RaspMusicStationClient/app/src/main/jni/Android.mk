LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := RaspMusicStationClient
LOCAL_SRC_FILES := RaspMusicStationClient.cpp

include $(BUILD_SHARED_LIBRARY)
