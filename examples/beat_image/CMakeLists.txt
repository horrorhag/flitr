PROJECT(flitr_beat_image)

SET(SOURCES
  beat_image_example.cpp
)

ADD_EXECUTABLE(flitr_beat_image ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_beat_image flitr)
