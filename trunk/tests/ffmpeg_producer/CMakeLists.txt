PROJECT(test_ffmpeg_producer)

SET(SOURCES
  test.cpp
)

ADD_EXECUTABLE(test_ffmpeg_producer ${SOURCES})
TARGET_LINK_LIBRARIES(test_ffmpeg_producer flitr)
