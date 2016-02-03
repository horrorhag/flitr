PROJECT(flitr_morphological_filter)

SET(SOURCES
  morphological_filter_example.cpp
)

ADD_EXECUTABLE(flitr_morphological_filter ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_morphological_filter flitr)
