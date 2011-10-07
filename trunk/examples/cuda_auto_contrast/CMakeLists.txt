PROJECT(flitr_cuda_auto_contrast)

SET(SOURCES
  cuda_auto_contrast.cpp
  cuda_auto_contrast_pass.cpp
  cuda_auto_contrast_pass.cu
  simple_shader_pass.cpp
)

CUDA_ADD_EXECUTABLE(flitr_cuda_auto_contrast ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_cuda_auto_contrast flitr)
