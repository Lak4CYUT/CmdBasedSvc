# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include_source =: $(LOCAL_PATH)/../inc/
include_source += $(TOP)/external/stlport/stlport/
include_source += $(TOP)/bionic/
include_source += $(TOP)/libcore/include/

LOCAL_SHARED_LIBRARIES := liblog libstlport 
LOCAL_STATIC_LIBRARIES := libcmdbasedsvc libcutils

LOCAL_MODULE:= test_srv

LOCAL_C_INCLUDES := $(include_source)

LOCAL_SRC_FILES += test.cpp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SHARED_LIBRARIES := liblog libstlport 
LOCAL_STATIC_LIBRARIES := libcmdbasedsvc libcutils

LOCAL_MODULE:= test_cnt

LOCAL_C_INCLUDES := $(include_source)

LOCAL_SRC_FILES += testcnt.cpp
include $(BUILD_EXECUTABLE)
