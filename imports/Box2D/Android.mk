LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE:= Box2D
LOCAL_SRC_FILES:= ./Box2D/libBox2D.a
include $(PREBUILT_STATIC_LIBRARY)
