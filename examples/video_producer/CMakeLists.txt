PROJECT(flitr_video_producer)

SET(SOURCES
  video_producer_example.cpp
)

ADD_EXECUTABLE(flitr_video_producer ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_video_producer flitr)
