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

#ifndef FLITR_TEXTURE_H
#define FLITR_TEXTURE_H 1

#include <osg/TextureRectangle>

#include <flitr/flitr_config.h>

#ifdef FLITR_WITH_OSGCUDA
    #include <osgCuda/Texture>
#endif

namespace flitr {

#ifdef FLITR_WITH_OSGCUDA
    typedef osgCuda::TextureRectangle TextureRectangle;
#else
    typedef osg::TextureRectangle TextureRectangle;
#endif

}

#endif //FLITR_TEXTURE_H
