/* Framework for Live Image Transformation (FLITr)
 * Copyright (c) 2010 CSIR
 *
 * This file is part of FLITr.
 *
 * FLITr is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * FLITr is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FLITr. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <iostream>
#include <memory>

using namespace std;

using std::shared_ptr;


#include <flitr/modules/flitr_image_processors/deinterlace/fip_deinterlace.h>


using namespace flitr;

//# ImageProcessor is initialised with default values next to the constructor using ":"
FIPDeinterlace::FIPDeinterlace(ImageProducer& upStreamProducer,
    uint32_t images_per_slot, uint32_t buffer_size, FLITR_DEINTERLACE_METHODS method)
    :
      ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
      _method(method)
    {
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat_.push_back(upStreamProducer.getFormat(i));//Output format is same as input format.
    }

}

///Helper functions
//Get median
float getMedian(float a, float b, float c);
//print matrix
void printMatrix(std::vector<std::vector<float>> matrix);

//Deconstructor
FIPDeinterlace::~FIPDeinterlace()
{
    /*
    for (uint32_t i=0; i<ImagesPerSlot_; ++i)
    {
        delete [] resultImageVec_[i];

        for (size_t historyIndex=0; historyIndex<windowLength_; ++historyIndex)
        {
            delete [] historyImageVecVec_[i][historyIndex];
        }
        historyImageVecVec_[i].clear();
    }

    resultImageVec_.clear();
    */
}

bool FIPDeinterlace::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.

    for (uint32_t i=0; i<ImagesPerSlot_; ++i)
    {
        const ImageFormat imFormat=getUpstreamFormat(i);//Downstream format is same as upstream format.

        //# Image resolution
        const size_t width=imFormat.getWidth();
        const size_t height=imFormat.getHeight();

        const size_t componentsPerPixel=imFormat.getComponentsPerPixel();   //# Get the image's dimension

        //resultImageVec_.push_back(new float[width*height*componentsPerPixel]);
        //memset(resultImageVec_[i], 0, width*height*componentsPerPixel*sizeof(float));

        //Three Neighbouring images for Deinterlace filter
        imReadPrevious = (Image*) malloc(width*height*componentsPerPixel*sizeof(float));

        //Prepare memory for the deinterlaced image
        dataDeinterlacedIm = (float*) malloc(width*height*componentsPerPixel*sizeof(float));

        currentImageVec_.push_back(new float[width*height*componentsPerPixel]);
        memset(currentImageVec_[i], 0, width*height*componentsPerPixel*sizeof(float));

        PixelsOnSamePosition = new float[2*componentsPerPixel];

        /*
        historyImageVecVec_.push_back(std::vector<float *>());
        for (size_t historyIndex=0; historyIndex<historyImageVecVec_.size(); ++historyIndex)
        {
            historyImageVecVec_[i].push_back(new float[width*height*componentsPerPixel]);
            memset(historyImageVecVec_[i][historyIndex], 0, width*height*componentsPerPixel*sizeof(float));
        }
        */
    }

    return rValue;
}


bool FIPDeinterlace::trigger()
{

    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {//There are images to consume and the downstream producer has space to produce.
        std::vector<Image**> imvRead=reserveReadSlot();
        std::vector<Image**> imvWrite=reserveWriteSlot();

        //Start stats measurement event.
        ProcessorStats_->tick();


        for (size_t imgNum=0; imgNum<ImagesPerSlot_; ++imgNum)      //#ImagesPerSlot_ is always = 1 here
        {
            //Reading the current image
            Image const * const imReadCurrent = *(imvRead[imgNum]);     //Read the current image
            Image * const imWrite = *(imvWrite[imgNum]);                //This will update the current image


            // My Own Deinterlace filter. Takes into consideration:
            //  -Vertical and Horizontal movement
            //  -Partial Diagonal Movements
            //std::vector<std::vector<float>> smoothFilter { {0.5,1,0.5},{1,2,1},{0.5,1,0.5}};  //You must divide the Results Image by 8
            std::vector<std::vector<float>> smoothFilter { {0.0,1,0.0},{1,0,1},{0.0,1,0.0}};


            const ImageFormat imFormat=getUpstreamFormat(imgNum);//Downstream format is same as upstream format.
            const size_t width=imFormat.getWidth();
            const size_t height=imFormat.getHeight();


            //Update this slot's average image here...
            if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_F32)        //Check the format of the image
            {

                //Copy the first frame. Then start programme from frame 1
                if(imageCount == 0){
                    *imReadPrevious = *(*imvRead[imgNum]);          //Store the Previous image
                    //memcpy(imReadPrevious ,imReadCurrent, width*height*componentsPerPixel*sizeof(float));
                }

                //Deinterlace using the 2 consecutive images, only work on every Previous image.
                if(imageCount >= 1){
                    // Read the data from the 2 images   //const float  * dataReadPrevious
                    const float  * const dataReadCurrent = (float const * const)imReadCurrent->data();
                    const float  * const dataReadPrevious =  (float const * const)imReadPrevious->data();             //(const float*) malloc(width*height*componentsPerPixel*sizeof(float));

                    //Prepare the memory for the data to be written
                    float * const dataWrite = (float * const)imWrite->data();

                    //Access the image
                    for (size_t y=0; y<height; ++y){

                        //The 2D image is converted to a 1D vector, So lineOffSet iters rows
                        const size_t rowOffset= y * width; //# iter rows

                        for (size_t x=0; x<width; ++x){

                            ///My Deinterlace 4x4 Filter, (Smooth Filter)
                            if(_method == flitr::FLITR_DEINTERLACE_METHODS::smoothFilter){

                                float sumFilter = 0;
                                //Apply filter
                                if( (x < (width-3)) && (y>1) && (y < (height-1)) ){

                                    for(int yf=0; yf<3;yf++){ //iter Filter rows

                                        const size_t filterOffset = (y+yf-1) * width;

                                        for(int xf=0; xf<3;xf++){ //iter Filter cols
                                            //printf("xf=%d yf=%d  \tdataRead[]=%.1f \n",xf,yf,dataRead[lineOffset + x]);
                                            sumFilter += (
                                             ((dataReadPrevious[filterOffset+ (x+xf)])  * (float) smoothFilter[yf][xf]) +
                                             ((dataReadCurrent[filterOffset+ (x+xf)])  * (float) smoothFilter[yf][xf])
                                             );
                                        }
                                    }

                                    //Result Image
                                    dataDeinterlacedIm[rowOffset + x] = sumFilter/(2*(6));    //Divide By 2 because we used 2 images, 8 is the sum of the filter
                                    dataWrite[rowOffset + x]= dataDeinterlacedIm[rowOffset + x] ;
                                }
                            }else{

                                /// Deinterlace Methods from Literature
                                //Pixel on the same position of the 2 consecutive image
                                PixelsOnSamePosition[0] = dataReadPrevious[rowOffset + x];
                                PixelsOnSamePosition[1] = dataReadCurrent[rowOffset + x];

                                if(_method == flitr::FLITR_DEINTERLACE_METHODS::linear){
                                    //Linear-Deinterlace //This currently deinterlaces from the 2nd image. Fix later
                                    if(((y+1)%2) == (imageCount%2) ){
                                        dataDeinterlacedIm[rowOffset + x] =  dataReadCurrent[rowOffset + x];
                                    }else{
                                        dataDeinterlacedIm[rowOffset + x] =
                                        (dataReadPrevious[rowOffset + x] + dataReadCurrent[rowOffset + x])/2.0;
                                    }

                                }else if(_method == flitr::FLITR_DEINTERLACE_METHODS::interframe){

                                    //Interframe Deinterlacing
                                    if(y > 1){
                                        if((rowOffset + x) < ((height*width)-1) && y<(height-1)){
                                            //Apply Vertical median-Deinterlace
                                            if(((y+1)%2) == (imageCount%2) ){
                                                dataDeinterlacedIm[rowOffset + x] =  dataReadCurrent[rowOffset + x];
                                            }else{
                                                dataDeinterlacedIm[rowOffset + x] = getMedian(dataReadCurrent[((y-1) * width) + x],
                                                dataReadCurrent[((y+1) * width) + x], dataReadPrevious[rowOffset + x]);
                                            }
                                            //printf("getMedian()=%.1f \n", dataDeinterlacedIm[rowOffset + x] );
                                        }
                                    }




                                }else{
                                    dataDeinterlacedIm[rowOffset + x] = dataReadCurrent[rowOffset + x];
                                }


                                //Result Image
                                dataWrite[rowOffset + x]= dataDeinterlacedIm[rowOffset + x];

                            }

                        } //end for x
                    }   //end for y



                    //Store the previous image and work on them
                    //This shifts Previous to Current image. Preparing for the next image
                    *imReadPrevious = *(*imvRead[imgNum]);          //Store the Previous image
                    //memcpy(imReadPrevious ,imReadCurrent, width*height*componentsPerPixel*sizeof(float));


                }
                imageCount = imageCount +1;

            }



        }



        //Stop stats measurement event.
        ProcessorStats_->tock();

        releaseWriteSlot();
        releaseReadSlot();

        return true;
    }

    return false;
}



//get median
float getMedian(float a, float b, float c){
    float biggest = a, smallest = a;

    //Determine the biggest num
    if(b > biggest) biggest = b;
    if(c > biggest) biggest = c;

    //Determine the smallest num
    if(b < smallest)    smallest = b;
    if(c < smallest)    smallest = c;

    return (a+b+c)-smallest-biggest;    //Return the median
}


//print matrix
void printMatrix(std::vector<std::vector<float>> matrix){
    int rows = (int) matrix.size();
    int cols = (int) matrix[0].size();

    printf("# rows=%d cols=%d\n",(int) rows, cols);

    for(int i =0; i<rows;i++){
        for(int j=0; j<cols;j++){
            printf("%f",(float) matrix[i][j] );
            if(j<cols-1)
                printf(",");
        }
        printf("\n");
    }

}
