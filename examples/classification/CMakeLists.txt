PROJECT(flitr_classification)

SET(SOURCES
  classification.cpp
)

#Remove this, just to debug (Dumi) =====================
#Add Cuda
INCLUDE_DIRECTORIES(/usr/local/cuda/include;/usr/local/cuda/targets/x86_64-linux/include/)

##============================
## Add Caffe
##============================
##Assign <Caffe_DIR> to a directory containing one of the above files, [CaffeConfig.cmake] or [caffe-config.cmake].
#SET(Caffe_DIR "/home/dumisani/dev/caffe/build/install/share/Caffe")  #CNN Classifier and RCNN

#find_package(Caffe)
#LINK_DIRECTORIES(${Caffe_LIBRARIES})
#include_directories(${Caffe_INCLUDE_DIRS})
#add_definitions(${Caffe_DEFINITIONS})    # ex. -DCPU_ONLY
#MESSAGE("${CAFFE_FOUND}")
#MESSAGE("Caffe_LIBRARIES: ${Caffe_LIBRARIES}")
#MESSAGE("Caffe_INCLUDE_DIRS : ${Caffe_INCLUDE_DIRS}")

#IF(Caffe_FOUND)
#    message("Caffe found!!")
#ELSE()
#    message("Caffe not found!!")
#ENDIF(Caffe_FOUND)
##=========================


#Add OpenCV 3
INCLUDE_DIRECTORIES(/usr/local/include)
LINK_DIRECTORIES(/usr/local/lib)


ADD_EXECUTABLE(flitr_classification ${SOURCES})
#LINK_DIRECTORIES(/usr/lib/x86_64-linux-gnu)  #For libboost_system.so
#LINK_DIRECTORIES(/usr/local/cuda/lib64;/usr/local/cuda-7.5/targets/x86_64-linux/lib/)


TARGET_LINK_LIBRARIES(flitr_classification flitr glog cuda ${opencv}  ${Caffe_LIBRARIES})
#opencv_core opencv_imgproc opencv_highgui lboost
#Mine (Worked): flitr_classification boost_system -lstdc++ flitr glog cuda opencv_core opencv_imgproc opencv_highgui caffe)

