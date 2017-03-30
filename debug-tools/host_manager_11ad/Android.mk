LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := host_manager_11ad

LOCAL_MODULE_TAGS := optional

LOCAL_CPPFLAGS := -Wall -lpthread -fexceptions

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/access_layer_11ad \
	$(LOCAL_PATH)/access_layer_11ad/Unix \

LOCAL_SRC_FILES := $(shell find $(LOCAL_PATH) -name '*.cpp' | sed s:^$(LOCAL_PATH)::g )

include $(BUILD_EXECUTABLE)
