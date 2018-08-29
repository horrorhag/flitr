PROJECT(flitr_opencv_processor)

SET(SOURCES
  opencv_processor_example.cpp
)

INCLUDE_DIRECTORIES(/usr/local/lib;usr/include/opencv;/usr/include/opencv2)

ADD_EXECUTABLE(flitr_opencv_processor ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_opencv_processor flitr ${OpenCV_LIBS})
