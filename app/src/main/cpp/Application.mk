#APP_STL := stlport_shared
#APP_STL := stlport_static
# for zxing, use gnustl_static
APP_STL := gnustl_static
APP_PROJECT_PATH := $(call my-dir)/../
LOCAL_C_INCLUDES += ${ndk}/sources/cxx-stl/gnu-libstdc++/4.8/include
APP_CFLAGS := -pthread -frtti -fexceptions -O3
APP_CPPFLAGS  := -std=c++11
LOCAL_ARM_NEON := true
APP_OPTIM := release
STLPORT_FORCE_REBUILD := true
APP_MODULES      := engine recogPassport
APP_PLATFORM := android-15
APP_ABI := armeabi-v7a

#armeabi-v7a