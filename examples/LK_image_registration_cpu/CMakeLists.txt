PROJECT(flitr_lk_image_registration_cpu)

SET(SOURCES
  lucas_kanade_test.cpp
)

ADD_EXECUTABLE(flitr_lk_image_registration_cpu ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_lk_image_registration_cpu flitr)

