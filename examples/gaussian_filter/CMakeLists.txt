PROJECT(flitr_gaussian_filter)

SET(SOURCES
  gaussian_filter_example.cpp
)

ADD_EXECUTABLE(flitr_gaussian_filter ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_gaussian_filter flitr)
