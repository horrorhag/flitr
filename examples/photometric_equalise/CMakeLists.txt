PROJECT(flitr_photometric_equalise)

SET(SOURCES
  photometric_equalise_example.cpp
)

ADD_EXECUTABLE(flitr_photometric_equalise ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_photometric_equalise flitr)
