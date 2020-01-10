# Test that LOCAL_CPPFLAGS only works for C++ sources
#
LOCAL_PATH:= $(call my-dir)


include $(CLEAR_VARS)
LOCAL_MODULE    := engine
LOCAL_C_INCLUDES := $(LOCAL_PATH)/engine
LOCAL_CPPFLAGS := -UBANANA -DBANANA=300 
LOCAL_SRC_FILES := \
	engine/StdAfx.cpp \
	engine/Binarization.cpp \
	engine/Histogram.cpp \
	engine/ImageBase.cpp \
	engine/ImageFilter.cpp \
	engine/Rotation.cpp \
	engine/RecogCore.cpp \
	engine/RecogMQDF.cpp \
	engine/Recog.cpp \
	engine/LineRecogPrint.cpp \
	engine/FindRecogDigit.cpp 
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := recogPassport
LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_CPPFLAGS := -UBANANA -DBANANA=300
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -ldl -lm -llog -ljnigraphics -landroid
LOCAL_SRC_FILES := \
	zinterface.cpp
LOCAL_STATIC_LIBRARIES := engine

include $(BUILD_SHARED_LIBRARY)



