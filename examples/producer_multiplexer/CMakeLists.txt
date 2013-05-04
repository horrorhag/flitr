PROJECT(flitr_producer_multiplexer)

SET(SOURCES
  producer_multiplexer_example.cpp
)

ADD_EXECUTABLE(flitr_producer_multiplexer ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_producer_multiplexer flitr)
