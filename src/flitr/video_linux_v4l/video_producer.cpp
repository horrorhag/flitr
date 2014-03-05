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

#include <flitr/video_producer.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/types.h>
#include <linux/videodev.h>
#include "ccvt.h"
#ifdef USE_EYETOY
#include "jpegtorgb.h"
#endif

using namespace flitr;

namespace flitr
{
struct VideoParam
{
  //device controls
    char                dev[256];
    int                 channel;
    int                 width;
    int                 height;
    int                 palette;
  //image controls
    double              brightness;
    double              contrast;
    double              saturation;
    double              hue;
    double              whiteness;

  //options controls
    int                 mode;

    int                 debug;

    int                 fd;
    int                 video_cont_num;
    ARUint8             *map;
    ARUint8             *videoBuffer;
    struct video_mbuf   vm;
    struct video_mmap   vmm;
};
}

namespace
{
    const int MAXCHANNEL = 10;
    static flitr::VideoParam* gVid = 0;
    const std::string DEFAULT_VIDEO_DEVICE = "/dev/video0";
    const int DEFAULT_VIDEO_WIDTH = 640;
    const int DEFAULT_VIDEO_HEIGHT = 480;
    const int DEFAULT_VIDEO_CHANNEL= 1;
    const int DEFAULT_VIDEO_MODE= VIDEO_MODE_NTSC;
}

VideoParam* VideoOpen( char *config_in );
int VideoClose( void );
int VideoCapStart( VideoParam *vid );
int VideoCapStop( VideoParam *vid );
int VideoCapNext( VideoParam *vid );
unsigned char *VideoGetImage( VideoParam *vid );

/**************************************************************
* Thread
***************************************************************/
void VideoProducer::VideoProducerThread::run()
{
    std::vector<Image**> aImages;
    
    while (!shouldExit)
    {
        // grab a vide frame
        unsigned char* pixelBuffer;
        if( (pixelBuffer = VideoGetImage(gVid)) == NULL ) 
        {
            Thread::microSleep(1000);
            continue;
        }

        std::vector<uint8_t> rgb(producer->ImageFormat_.back().getBytesPerImage());
        uint8_t* buffer_vid = static_cast<uint8_t*>(pixelBuffer);
        std::copy(buffer_vid, buffer_vid + producer->ImageFormat_.back().getBytesPerImage(), rgb.begin());

        // wait until there is space in the write buffer so that we can get a valid image pointer
        do
        {
            aImages = producer->reserveWriteSlot();
            if (aImages.size() != 1)
            {
                Thread::microSleep(1000);
            }
        } while ((aImages.size() != 1) && !shouldExit);

        if (!shouldExit)
        {
            Image* img = *(aImages[0]);
            uint8_t* buffer = img->data();
            std::copy(rgb.begin(), rgb.end(), buffer);
        }

        producer->releaseWriteSlot();

        Thread::microSleep(1000);

        VideoCapNext(gVid);
    }   
}

/**************************************************************
* Producer
***************************************************************/

VideoProducer::VideoProducer()
    : thread(0)
    , imageSlots(10)
{
}

VideoProducer::~VideoProducer()
{
    VideoCapStop(gVid);
    VideoClose();
}

bool VideoProducer::init()
{
    gVid = VideoOpen();

    VideoCapStart(gVid);

    return true;
}

VideoParam* VideoOpen( char *config_in )
{
    VideoParam*				  vid;
    struct video_capability   vd;
    struct video_channel      vc[MAXCHANNEL];
    struct video_picture      vp;
    char                      *config, *a, line[256];
    int                       i;
    int                       adjust = 1;


    /* If no config string is supplied, we should use the environment variable, otherwise set a sane default */
    if (!config_in || !(config_in[0])) {
        config = NULL;
        printf ("No video config string supplied, using defaults.\n");
        
    } else {
        config = config_in;
        printf ("Using supplied video config string [%s].\n", config_in);
    }
    
    vid = new flitr::VideoParam;
    strcpy( vid->dev, DEFAULT_VIDEO_DEVICE );
    vid->channel    = DEFAULT_VIDEO_CHANNEL; 
    vid->width      = DEFAULT_VIDEO_WIDTH;
    vid->height     = DEFAULT_VIDEO_HEIGHT;
    vid->palette = VIDEO_PALETTE_RGB24;
    vid->contrast   = -1.;
    vid->brightness = -1.;
    vid->saturation = -1.;
    vid->hue        = -1.;
    vid->whiteness  = -1.;
    vid->mode       = DEFAULT_VIDEO_MODE;
    vid->debug      = 0;
    vid->videoBuffer=NULL;

    vid->fd = open(vid->dev, O_RDWR);// O_RDONLY ?
    if(vid->fd < 0){
        printf("video device (%s) open failed\n",vid->dev); 
        free( vid );
        return 0;
    }

    if(ioctl(vid->fd,VIDIOCGCAP,&vd) < 0){
        printf("ioctl failed\n");
        free( vid );
        return 0;
    }

    if( vid->debug ) {
        printf("=== debug info ===\n");
        printf("  vd.name      =   %s\n",vd.name);
        printf("  vd.channels  =   %d\n",vd.channels);
        printf("  vd.maxwidth  =   %d\n",vd.maxwidth);
        printf("  vd.maxheight =   %d\n",vd.maxheight);
        printf("  vd.minwidth  =   %d\n",vd.minwidth);
        printf("  vd.minheight =   %d\n",vd.minheight);
    }
    
    /* adjust capture size if needed */
    if (adjust)
      {
    if (vid->width >= vd.maxwidth)
      vid->width = vd.maxwidth;
    if (vid->height >= vd.maxheight)
      vid->height = vd.maxheight;
    if (vid->debug)
      printf ("VideoOpen: width/height adjusted to (%d, %d)\n", vid->width, vid->height);
      }
    
    /* check capture size */
    if(vd.maxwidth  < vid->width  || vid->width  < vd.minwidth ||
       vd.maxheight < vid->height || vid->height < vd.minheight ) {
        printf("VideoOpen: width or height oversize \n");
        free( vid );
        return 0;
    }
    
    /* adjust channel if needed */
    if (adjust)
      {
    if (vid->channel >= vd.channels)
      vid->channel = 0;
    if (vid->debug)
      printf ("VideoOpen: channel adjusted to 0\n");
      }
    
    /* check channel */
    if(vid->channel < 0 || vid->channel >= vd.channels){
        printf("VideoOpen: channel# is not valid. \n");
        free( vid );
        return 0;
    }

    if( vid->debug ) {
        printf("==== capture device channel info ===\n");
    }

    for(i = 0;i < vd.channels && i < MAXCHANNEL; i++){
        vc[i].channel = i;
        if(ioctl(vid->fd,VIDIOCGCHAN,&vc[i]) < 0){
            printf("error: acquireing channel(%d) info\n",i);
            free( vid );
            return 0;
        }

        if( vid->debug ) {
            printf("    channel = %d\n",  vc[i].channel);
            printf("       name = %s\n",  vc[i].name);
            printf("     tuners = %d",    vc[i].tuners);

            printf("       flag = 0x%08x",vc[i].flags);
            if(vc[i].flags & VIDEO_VC_TUNER) 
                printf(" TUNER");
            if(vc[i].flags & VIDEO_VC_AUDIO) 
                printf(" AUDIO");
            printf("\n");

            printf("     vc[%d].type = 0x%08x", i, vc[i].type);
            if(vc[i].type & VIDEO_TYPE_TV) 
                printf(" TV");
            if(vc[i].type & VIDEO_TYPE_CAMERA) 
                printf(" CAMERA");
            printf("\n");       
        }
    }

    /* select channel */
    vc[vid->channel].norm = vid->mode;       /* 0: PAL 1: NTSC 2:SECAM 3:AUTO */
    if(ioctl(vid->fd, VIDIOCSCHAN, &vc[vid->channel]) < 0){
        printf("error: selecting channel %d\n", vid->channel);
        free( vid );
        return 0;
    }

    if(ioctl(vid->fd, VIDIOCGPICT, &vp)) {
        printf("error: getting palette\n");
       free( vid );
       return 0;
    }

    if( vid->debug ) {
        printf("=== debug info ===\n");
        printf("  vp.brightness=   %d\n",vp.brightness);
        printf("  vp.hue       =   %d\n",vp.hue);
        printf("  vp.colour    =   %d\n",vp.colour);
        printf("  vp.contrast  =   %d\n",vp.contrast);
        printf("  vp.whiteness =   %d\n",vp.whiteness);
        printf("  vp.depth     =   %d\n",vp.depth);
        printf("  vp.palette   =   %d\n",vp.palette);
    }

    /* set video picture */
    if ((vid->brightness+1.)>0.001)
    vp.brightness   = 32767 * 2.0 *vid->brightness;
    if ((vid->contrast+1.)>0.001)
    vp.contrast   = 32767 * 2.0 *vid->contrast;
    if ((vid->hue+1.)>0.001)
    vp.hue   = 32767 * 2.0 *vid->hue;
    if ((vid->whiteness+1.)>0.001)
    vp.whiteness   = 32767 * 2.0 *vid->whiteness;
    if ((vid->saturation+1.)>0.001)
    vp.colour   = 32767 * 2.0 *vid->saturation;
    vp.depth      = 24;    
    vp.palette    = vid->palette;

    if(ioctl(vid->fd, VIDIOCSPICT, &vp)) {
        printf("error: setting configuration !! bad palette mode..\n");
        free( vid );
        return 0;
    }
    if (vid->palette==VIDEO_PALETTE_YUV420P)
        arMalloc( vid->videoBuffer, ARUint8, vid->width*vid->height*3 );

    if( vid->debug ) { 
        if(ioctl(vid->fd, VIDIOCGPICT, &vp)) {
            printf("error: getting palette\n");
            free( vid );
            return 0;
        }
        printf("=== debug info ===\n");
        printf("  vp.brightness=   %d\n",vp.brightness);
        printf("  vp.hue       =   %d\n",vp.hue);
        printf("  vp.colour    =   %d\n",vp.colour);
        printf("  vp.contrast  =   %d\n",vp.contrast);
        printf("  vp.whiteness =   %d\n",vp.whiteness);
        printf("  vp.depth     =   %d\n",vp.depth);
        printf("  vp.palette   =   %d\n",vp.palette);
    }

    /* get mmap info */
    if(ioctl(vid->fd,VIDIOCGMBUF,&vid->vm) < 0){
        printf("error: videocgmbuf\n");
        free( vid );
        return 0;
    }

    if( vid->debug ) {
        printf("===== Image Buffer Info =====\n");
        printf("   size   =  %d[bytes]\n", vid->vm.size);
        printf("   frames =  %d\n", vid->vm.frames);
    }
    if(vid->vm.frames < 2){
        printf("this device can not be supported by libARvideo.\n");
        printf("(vm.frames < 2)\n");
        free( vid );
        return 0;
    }


    /* get memory mapped io */
    if((vid->map = (ARUint8 *)mmap(0, vid->vm.size, PROT_READ|PROT_WRITE, MAP_SHARED, vid->fd, 0)) < 0){
        printf("error: mmap\n");
        free( vid );
        return 0;
    }

    /* setup for vmm */ 
    vid->vmm.frame  = 0;
    vid->vmm.width  = vid->width;
    vid->vmm.height = vid->height;
    vid->vmm.format= vid->palette;

    vid->video_cont_num = -1;

#ifdef USE_EYETOY
    JPEGToRGBInit(vid->width,vid->height);
#endif
    return vid;
}

int VideoClose( void )
{
    int result;
    
    if( gVid == NULL ) return -1;

    result = ar2VideoClose(gVid);
    gVid = NULL;
    return (result);
}

int VideoCapStart( VideoParam *vid )
{
    if(vid->video_cont_num >= 0){
        printf("arVideoCapStart has already been called.\n");
        return -1;
    }

    vid->video_cont_num = 0;
    vid->vmm.frame      = vid->video_cont_num;
    if(ioctl(vid->fd, VIDIOCMCAPTURE, &vid->vmm) < 0) {
        return -1;
    }
    vid->vmm.frame = 1 - vid->vmm.frame;
    if( ioctl(vid->fd, VIDIOCMCAPTURE, &vid->vmm) < 0) {
        return -1;
    }

    return 0;
}

int VideoCapStop( VideoParam *vid )
{
    if(vid->video_cont_num < 0){
        printf("arVideoCapStart has never been called.\n");
        return -1;
    }
    if(ioctl(vid->fd, VIDIOCSYNC, &vid->video_cont_num) < 0){
        printf("error: videosync\n");
        return -1;
    }
    vid->video_cont_num = -1;

    return 0;
}

int VideoCapNext( VideoParam *vid )
{
    if(vid->video_cont_num < 0){
        printf("arVideoCapStart has never been called.\n");
        return -1;
    }

    vid->vmm.frame = 1 - vid->vmm.frame;
    ioctl(vid->fd, VIDIOCMCAPTURE, &vid->vmm);

    return 0;
}

unsigned char *VideoGetImage( VideoParam *vid )
{
    ARUint8 *buf;

    if(vid->video_cont_num < 0){
        printf("arVideoCapStart has never been called.\n");
        return NULL;
    }

    if(ioctl(vid->fd, VIDIOCSYNC, &vid->video_cont_num) < 0){
        printf("error: videosync\n");
        return NULL;
    }
    vid->video_cont_num = 1 - vid->video_cont_num;

    if(vid->video_cont_num == 0)
        buf=(vid->map + vid->vm.offsets[1]); 
    else
        buf=(vid->map + vid->vm.offsets[0]);
    
    if(vid->palette == VIDEO_PALETTE_YUV420P)
    {

        /* ccvt_420p_bgr24(vid->width, vid->height, buf, buf+(vid->width*vid->height),
                buf+(vid->width*vid->height)+(vid->width*vid->height)/4,
                vid->videoBuffer);
        */

        ccvt_420p_bgr24(vid->width, vid->height, buf, vid->videoBuffer);

        return vid->videoBuffer;
    }
#ifdef USE_EYETOY
    buf=JPEGToRGB(buf,vid->width, vid->height);
#endif

    return buf;

}