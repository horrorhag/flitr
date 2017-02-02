PROJECT(flitr_motion_detect)

SET(SOURCES
  motion_detect_example.cpp
)

ADD_EXECUTABLE(flitr_motion_detect ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_motion_detect flitr)
