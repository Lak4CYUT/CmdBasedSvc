# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := src/CmdServerV1.cpp
LOCAL_SRC_FILES += src/Comm.cpp

LOCAL_C_INCLUDES += $(LOCAL_PATH)/src/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/inc/
LOCAL_C_INCLUDES += $(TOP)/bionic/
LOCAL_C_INCLUDES += $(TOP)/libcore/include/
LOCAL_C_INCLUDES += $(TOP)/system/core/include/
include external/stlport/libstlport.mk

LOCAL_SHARED_LIBRARIES += liblog

LOCAL_STATIC_LIBRARIES += libcutils

LOCAL_MODULE:= libcmdbasedsvc
LOCAL_MODULE_TAGS:= optional

LOCAL_PRELINK_MODULE := true
LOCAL_CFLAGS += -fpermissive
include $(BUILD_STATIC_LIBRARY)

###### The other version, use gunstl.
include $(CLEAR_VARS)
LOCAL_SRC_FILES := src/CmdServerV1.cpp
LOCAL_SRC_FILES += src/Comm.cpp

LOCAL_C_INCLUDES += $(LOCAL_PATH)/src/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/inc/
LOCAL_C_INCLUDES += $(TOP)/bionic/
LOCAL_C_INCLUDES += $(TOP)/system/core/include/

LOCAL_SHARED_LIBRARIES := liblog
 
LOCAL_STATIC_LIBRARIES := libcutils

# Use gunstl static library
LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 18

LOCAL_MODULE:= libcmdbasedsvc_gunstl
LOCAL_MODULE_TAGS:= optional

LOCAL_PRELINK_MODULE := true
LOCAL_CFLAGS += -fpermissive
include $(BUILD_STATIC_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))