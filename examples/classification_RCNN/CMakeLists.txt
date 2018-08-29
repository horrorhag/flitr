PROJECT(flitr_classification_RCNN)

#===============================
#Find OpenCV 3
# --searching where there is "OpenCVConfig.cmake.in"
#SET(OpenCV_DIR "/home/dumisani/dev/openCV3/cmake")

#// Removed this, it finds opencv 2 instead of 3
#find_package(OpenCV HINTS
#    /home/dumisani/dev/openCV3/cmake,
#    /usr/local/opt/opencv3)
#find_package(OpenCV 3 REQUIRED )
#INCLUDE_DIRECTORIES(~/dev/openCV3/build/include)
##INCLUDE_DIRECTORIES(~/dev/openCV3/opencv_contrib-master/modules/dnn/include)
#LINK_DIRECTORIES(~/dev/openCV3/build/lib)


##=== From /usr/local ==========================
INCLUDE_DIRECTORIES(/usr/local/include/)
LINK_DIRECTORIES(/usr/local/lib)


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



SET(SOURCES
 classification_RCNN.cpp
)




INCLUDE_DIRECTORIES(/usr/local/include)
LINK_DIRECTORIES(/usr/local/lib)
LINK_DIRECTORIES(/usr/lib/x86_64-linux-gnu/)

ADD_EXECUTABLE(flitr_classification_RCNN ${SOURCES})
#LINK_DIRECTORIES(/usr/local/cuda/lib64;/usr/local/cuda-7.5/targets/x86_64-linux/lib/)
TARGET_LINK_LIBRARIES(flitr_classification_RCNN flitr boost_system glog cuda opencv_core opencv_imgproc opencv_highgui opencv_dnn ${Caffe_LIBRARIES})
## -lstdc++
#WORKED  (flitr_classification_RCNN flitr boost_system glog cuda caffe opencv_core opencv_imgproc opencv_highgui opencv_dnn)
