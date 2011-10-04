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

#include <iostream>
#include <string>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osg/io_utils>

#include <flitr/ffmpeg_producer.h>
#include <flitr/multi_osg_consumer.h>
#include <flitr/textured_quad.h>
#include <flitr/manipulator_utils.h>
#include <flitr/ortho_texture_manipulator.h>

#include "cuda_auto_contrast_pass.h"

using std::tr1::shared_ptr;
using namespace flitr;

int main(int argc, char *argv[])
{
    osg::ArgumentParser arguments(&argc, argv);

    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " video_file [--stretch]\n";
        std::cout << "Default is histogram equalisation, use --stretch for contrast stretch.\n";
        return 1;
    }

    shared_ptr<FFmpegProducer> ffp(new FFmpegProducer(argv[1], ImageFormat::FLITR_PIX_FMT_Y_8));
    if (!ffp->init()) {
        std::cerr << "Could not load " << argv[1] << "\n";
        exit(-1);
    }

    osg::Group* root_node = new osg::Group;

    shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*ffp,1));
    if (!osgc->init()) {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }

    bool useStretch = false;
    while (arguments.read("--stretch")) { useStretch = true; }
    
    // Test CUDA auto contrast
    ImageFormat imgFormat = ffp->getFormat();
    CUDAAutoContrastPass* CUDAPass;
    if (useStretch) {
        CUDAPass = new CUDAAutoContrastPass(
        osgc->getOutputTexture(0,0), 
        imgFormat,
        CUDAAutoContrastPass::CONTRAST_STRETCH);
    } else {
        CUDAPass = new CUDAAutoContrastPass(
        osgc->getOutputTexture(0,0), 
        imgFormat,
        CUDAAutoContrastPass::HISTOGRAM_EQUALISATION);
    }
    root_node->addChild(CUDAPass->getRoot().get());
    
    //shared_ptr<TexturedQuad> quad(new TexturedQuad(osgc->getOutputTexture()));
    shared_ptr<TexturedQuad> quad(new TexturedQuad(CUDAPass->getOutputTexture()));

    root_node->addChild(quad->getRoot().get());

    osgViewer::Viewer viewer;
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.setSceneData(root_node);

    //viewer.setUpViewInWindow(480+40, 40, 640, 480);
    viewer.realize();

    const int use_trackball = 0;
    if (use_trackball) {
        osgGA::TrackballManipulator* tb = new osgGA::TrackballManipulator;
        viewer.setCameraManipulator(tb);
        adjustCameraManipulatorHomeForYUp(tb);
    } else {
       
        ImageFormat imf = ffp->getFormat();
        OrthoTextureManipulator* om = new OrthoTextureManipulator(imf.getWidth(), imf.getHeight());
        viewer.setCameraManipulator(om);
    }

    while(!viewer.done()) {
        ffp->trigger();
        if (osgc->getNext()) {
            viewer.frame();
        } else {
            OpenThreads::Thread::microSleep(5000);
        }
    }

    return 0;
}
