PROJECT(test_shared_image_buffer)

SET(SOURCES
  test.cpp
)

ADD_EXECUTABLE(test_shared_image_buffer ${SOURCES})
TARGET_LINK_LIBRARIES(test_shared_image_buffer flitr ${FFmpeg_LIBRARIES})
