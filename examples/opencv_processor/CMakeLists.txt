PROJECT(flitr_opencv_processor)

SET(SOURCES
  opencv_processor_example.cpp
)

INCLUDE_DIRECTORIES(/usr/local/lib;usr/include/opencv;/usr/include/opencv2) #/usr/local/include/opencv;/usr/local/include/opencv2;)
#LINK_LIBRARIES(/usr/local/lib/libopencv_core.so;/usr/local/lib/libopencv_highgui.so)

ADD_EXECUTABLE(flitr_opencv_processor ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_opencv_processor flitr opencv_core  opencv_imgproc opencv_highgui)
