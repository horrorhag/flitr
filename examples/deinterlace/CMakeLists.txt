PROJECT(flitr_deinterlace)

SET(SOURCES
  deinterlace_example.cpp
)

ADD_EXECUTABLE(flitr_deinterlace ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_deinterlace flitr)
