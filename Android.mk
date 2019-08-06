
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

#lvgl
FILE_LIST := $(wildcard $(LOCAL_PATH)/lvgl/src/lv_core/*.c)
FILE_LIST += $(wildcard $(LOCAL_PATH)/lvgl/src/lv_draw/*.c)
FILE_LIST += $(wildcard $(LOCAL_PATH)/lvgl/src/lv_font/*.c)
FILE_LIST += $(wildcard $(LOCAL_PATH)/lvgl/src/lv_hal/*.c)
FILE_LIST += $(wildcard $(LOCAL_PATH)/lvgl/src/lv_misc/*.c)
FILE_LIST += $(wildcard $(LOCAL_PATH)/lvgl/src/lv_objx/*.c)
FILE_LIST += $(wildcard $(LOCAL_PATH)/lvgl/src/lv_themes/*.c)
#lv_drivers
FILE_LIST += $(wildcard $(LOCAL_PATH)/lv_drivers/*.c)
FILE_LIST += $(wildcard $(LOCAL_PATH)/lv_drivers/display/*.c)
FILE_LIST += $(wildcard $(LOCAL_PATH)/lv_drivers/indev/*.c)

FILE_LIST += $(wildcard $(LOCAL_PATH)/lv_examples/lv_apps/benchmark/*.c)
FILE_LIST += $(wildcard $(LOCAL_PATH)/lv_examples/lv_apps/demo/*.c)
FILE_LIST += $(wildcard $(LOCAL_PATH)/lv_examples/lv_apps/sysmon/*.c)
FILE_LIST += $(wildcard $(LOCAL_PATH)/lv_examples/lv_apps/terminal/*.c)
FILE_LIST += $(wildcard $(LOCAL_PATH)/lv_examples/lv_apps/tpcal/*.c)
FILE_LIST += $(wildcard $(LOCAL_PATH)/lv_examples/lv_tests/lv_test_theme/*.c)

FILE_LIST += $(wildcard $(LOCAL_PATH)/lv_examples/lv_tutorial/*/*.c)

LOCAL_SRC_FILES := $(FILE_LIST:$(LOCAL_PATH)/%=%)
LOCAL_SRC_FILES += main.c
LOCAL_C_INCLUDES +=\
    system/core/adf/libadf/include

LOCAL_SHARED_LIBRARIES := libc \
                          libcutils \
                          liblog \
                          libutils
LOCAL_WHOLE_STATIC_LIBRARIES += libadf
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE := levgl_demo

include $(BUILD_EXECUTABLE)

