PROJECT(flitr_cuda_histogram_equalisation)

SET(SOURCES
  cuda_histogram_equalisation.cpp
  cuda_histogram_equalisation_pass.cpp
  cuda_histogram_equalisation_pass.cu
)

CUDA_ADD_EXECUTABLE(flitr_cuda_histogram_equalisation ${SOURCES})
TARGET_LINK_LIBRARIES(flitr_cuda_histogram_equalisation flitr)
