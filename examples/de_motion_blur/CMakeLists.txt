PROJECT(flitr_de_motion_blur)

SET(SOURCES
  de_motion_blur_example.cpp
)

ADD_EXECUTABLE(flitr_de_motion_blur ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_de_motion_blur flitr)
