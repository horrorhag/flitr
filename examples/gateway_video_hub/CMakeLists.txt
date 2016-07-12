PROJECT(flitr_gateway_video_hub)

SET(SOURCES
  gateway_video_hub.cpp
  flitr_video_hub.cpp
  flitr_video_hub.h
)

ADD_EXECUTABLE(flitr_gateway_video_hub ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_gateway_video_hub flitr)
