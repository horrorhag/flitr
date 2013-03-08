PROJECT(flitr_save_texture_to_video_file)

SET(SOURCES
  save_texture_to_video_file_example.cpp
)

ADD_EXECUTABLE(flitr_save_texture_to_video_file ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_save_texture_to_video_file flitr)
