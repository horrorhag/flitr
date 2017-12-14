PROJECT(object_detection_CNN)

##=== From /usr/local ==========================
INCLUDE_DIRECTORIES(/usr/local/include/)
LINK_DIRECTORIES(/usr/local/lib)


#=====================================
# Link Caffe to the project

##Set <Caffe_DIR> to a directory containing one of the above files, [CaffeConfig.cmake] or [caffe-config.cmake].
#SET(Caffe_DIR "/home/dumisani/dev/caffe_SSD/caffe/build/install/share/Caffe")

#find_package(Caffe)
#include_directories(${Caffe_INCLUDE_DIRS})
#add_definitions(${Caffe_DEFINITIONS})    # ex. -DCPU_ONLY

#MESSAGE("Caffe_LIBRARIES: ${Caffe_LIBRARIES} \n ${CAFFE_LIBS}\n")
#MESSAGE("Caffe_INCLUDE_DIRS: ${Caffe_INCLUDE_DIRS}")

#IF(CAFFE_FOUND)
#        INCLUDE_DIRECTORIES(${CAFFE_INCLUDE_DIR})
#        LINK_DIRECTORIES(${CAFFE_LIBS})
#        add_definitions(${Caffe_DEFINITIONS})
#ELSE(CAFFE_FOUND)
#        MESSAGE("Caffe not found!!")
#ENDIF(CAFFE_FOUND)



SET(SOURCES
 object_detection_CNN.cpp
)



INCLUDE_DIRECTORIES(/usr/local/include)
LINK_DIRECTORIES(/usr/local/lib)
LINK_DIRECTORIES(/usr/lib/x86_64-linux-gnu/)

ADD_EXECUTABLE(object_detection_CNN ${SOURCES})
#LINK_DIRECTORIES(/usr/local/cuda/lib64;/usr/local/cuda-7.5/targets/x86_64-linux/lib/)
TARGET_LINK_LIBRARIES(object_detection_CNN flitr boost_system glog cuda ${Caffe_LIBRARIES} ${OpenCV_LIBRARY})
## -lstdc++
#TARGET_LINK_LIBRARIES(flitr_classification avformat flitr  boost_system glog cuda caffe opencv_core opencv_imgproc opencv_highgui )
