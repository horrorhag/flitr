PROJECT(flitr_screen_capture)

SET(SOURCES
  screen_capture.cpp
)

ADD_EXECUTABLE(flitr_screen_capture ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_screen_capture flitr)
