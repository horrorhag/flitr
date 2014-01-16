PROJECT(flitr_average_image)

SET(SOURCES
  average_image_example.cpp
)

ADD_EXECUTABLE(flitr_average_image ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_average_image flitr)
