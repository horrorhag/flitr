PROJECT(flitr_lk_image_registration)

SET(SOURCES
  lucas_kanade_test.cpp
)

ADD_EXECUTABLE(flitr_lk_image_registration ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_lk_image_registration flitr)
