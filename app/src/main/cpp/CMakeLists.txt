# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

set(JNI_ENGINE_INCLUDE engine)

include_directories(${JNI_ENGINE_INCLUDE} include)


add_library( recogPassport SHARED
			zinterface.cpp
			engine/StdAfx.cpp 
			engine/Binarization.cpp 
			engine/Histogram.cpp 
			engine/ImageBase.cpp 
			engine/ImageFilter.cpp 
			engine/Rotation.cpp 
			engine/RecogCore.cpp 
			engine/RecogMQDF.cpp 
			engine/Recog.cpp 
			engine/LineRecogPrint.cpp 
			engine/FindRecogDigit.cpp )

target_link_libraries(recogPassport
                      android
                      jnigraphics
                      z
                      m
                      dl
                      log)