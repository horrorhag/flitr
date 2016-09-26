PROJECT(flitr_histogram_processing)

SET(SOURCES
  main.cpp
)

ADD_EXECUTABLE(flitr_histogram_processing ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_histogram_processing flitr)
