LOCAL_PATH	:= $(call my-dir)/../..
include $(CLEAR_VARS)

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/.. \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/include/libuvc

LOCAL_EXPORT_C_INCLUDES := \
	$(LOCAL_PATH)/ \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/include/libuvc

LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%)
LOCAL_CFLAGS += -DANDROID_NDK
LOCAL_CFLAGS += -DLOG_NDEBUG
LOCAL_CFLAGS += -DUVC_DEBUGGING

LOCAL_EXPORT_LDLIBS := -llog

LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES += usb

LOCAL_SRC_FILES := \
	src/ctrl.c \
	src/device.c \
	src/diag.c \
	src/frame.c \
	src/init.c \
	src/stream.c

#LOCAL_MODULE := libuvc_static
#include $(BUILD_STATIC_LIBRARY)

######################################################################
# libuvc.so
######################################################################
#include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_EXPORT_LDLIBS += -llog

#LOCAL_WHOLE_STATIC_LIBRARIES = libuvc_static
#LOCAL_DISABLE_FATAL_LINKER_WARNINGS := true

LOCAL_MODULE := uvc
include $(BUILD_SHARED_LIBRARY)
