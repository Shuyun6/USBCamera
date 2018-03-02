LOCAL_PATH	:= $(call my-dir)
include $(CLEAR_VARS)

CFLAGS := -Werror

#添加头文件
#
LOCAL_C_INCLUDES := \
		$(LOCAL_PATH)/ \
		$(LOCAL_PATH)/../ \
		$(LOCAL_PATH)/../rapidjson/include \

LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%)

#在所有源文件中添加一个宏定义#define xxx（LOCAL_CFLAGS += -DXXX
#
LOCAL_CFLAGS += -DANDROID_NDK
LOCAL_CFLAGS += -DLOG_NDEBUG
LOCAL_CFLAGS += -DACCESS_RAW_DESCRIPTORS
LOCAL_CFLAGS += -O3 -fstrict-aliasing -fprefetch-loop-arrays

LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -ldl
LOCAL_LDLIBS += -llog
LOCAL_LDLIBS += -landroid

LOCAL_SHARED_LIBRARIES += usb uvc

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
		pupilla.cpp \
		UVCCamera.cpp \


LOCAL_MODULE    := UVCCamera

include $(BUILD_SHARED_LIBRARY)
