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

#include <flitr/modules/flitr_image_processors/dpt/fip_dpt.h>

#include <iostream>

using namespace flitr;
using std::shared_ptr;

FIPDPT::FIPDPT(ImageProducer& upStreamProducer, int32_t filterPulseSize,
               uint32_t images_per_slot,
               uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
filterPulseSize_(filterPulseSize)
{
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        //ImageFormat(uint32_t w=0, uint32_t h=0, PixelFormat pix_fmt=FLITR_PIX_FMT_Y_8, bool flipV = false, bool flipH = false):
        ImageFormat downStreamFormat(upStreamProducer.getFormat().getWidth(), upStreamProducer.getFormat().getHeight(),
                                     upStreamProducer.getFormat().getPixelFormat());
        
        ImageFormat_.push_back(downStreamFormat);
    }
    
    nodeVect_.reserve(upStreamProducer.getFormat().getWidth() * upStreamProducer.getFormat().getHeight());
    nodeVect_.resize(upStreamProducer.getFormat().getWidth() * upStreamProducer.getFormat().getHeight());
    
#ifdef RUN_ARCS
    arcVect_.reserve(upStreamProducer.getFormat().getWidth() * upStreamProducer.getFormat().getHeight() * 4);
    arcVect_.resize(upStreamProducer.getFormat().getWidth() * upStreamProducer.getFormat().getHeight() * 4);
#endif
    
    potentiallyActiveNodeIndexVect_.reserve(upStreamProducer.getFormat().getWidth() * upStreamProducer.getFormat().getHeight());
}

FIPDPT::~FIPDPT()
{}

bool FIPDPT::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    return rValue;
}

#ifdef RUN_ARCS
size_t FIPDPT::mergeArc(const int32_t arcIndex)
{
    size_t numPulsesMerged=0;
    Arc &arc=arcVect_[arcIndex];
    
    if (arc.active_)
    {
        const auto index0 = arc.nodeIndices_[0];
        const auto index1 = arc.nodeIndices_[1];
        Node &node0       = nodeVect_[index0];
        Node &node1       = nodeVect_[index1];
        
        if (node0.value_ == node1.value_)
        {
            node0.size_+=node1.size_;
            node0.pixelIndices_.insert(node0.pixelIndices_.end(),
                                       node1.pixelIndices_.begin(),
                                       node1.pixelIndices_.end());
            
            arc.active_=false;//Deactivate the now null link!
            
            for (const auto arcFromNode1Index : node1.arcIndices_)
            {
                Arc &arcFromNode1=arcVect_[arcFromNode1Index];
                
                if (arcFromNode1.active_)
                {
                    const size_t neighbourOfNode1Index=retrieveNeighbourNodeIndex(arcFromNode1, index1);
                    
                    if (neighbourOfNode1Index!=index0)
                    {
                        bool neighbourOfNode1AlreadyNeighbourOfNode0=false;
                        
                        for (const auto arcFromNode0Index : node0.arcIndices_)
                        {
                            Arc &arcFromNode0=arcVect_[arcFromNode0Index];
                            
                            const size_t neighbourOfNode0Index=retrieveNeighbourNodeIndex(arcFromNode0, index0);
                            
                            if (neighbourOfNode0Index==neighbourOfNode1Index)
                            {//Node0 and node1 already neighbours.
                                neighbourOfNode1AlreadyNeighbourOfNode0=true;
                                break;
                            }
                        }
                        
                        if (!neighbourOfNode1AlreadyNeighbourOfNode0)
                        {
                            node0.arcIndices_.push_back(arcFromNode1Index);
                            
                            if (arcFromNode1.nodeIndices_[0]==index1)
                            {
                                arcFromNode1.nodeIndices_[0]=index0;
                            } else
                                if (arcFromNode1.nodeIndices_[1]==index1)
                                {
                                    arcFromNode1.nodeIndices_[1]=index0;
                                }
                        } else
                        {//De-activate redundent arcs.
                            arcFromNode1.active_=false;
                        }
                    } else
                    {//De-activate redundent arcs.
                        arcFromNode1.active_=false;
                    }
                }
            }
            
            node1.size_=0;
            ++numPulsesMerged;
        }
    }
    
    return numPulsesMerged;
}
#else
size_t FIPDPT::mergeNode(const int32_t index0)
{
    size_t numPulsesMerged=0;
    Node &node0 = nodeVect_[index0];
    
    neighbourIndicesToAdd_.clear();
    
    for (int32_t &index1 : node0.neighbourIndices_)
    {
        Node &node1 = nodeVect_[index1];
        
        if (node1.size_>0)
        {
            if (node0.value_ == node1.value_)
            {//Merge node0 and node1.
                node0.size_+=node1.size_;
                node0.pixelIndices_.insert(node0.pixelIndices_.end(),
                                           node1.pixelIndices_.begin(),
                                           node1.pixelIndices_.end());
                
                
                for (const int32_t neighbourOfNode1Index : node1.neighbourIndices_)
                {
                    Node &neighbourOfNode1 = nodeVect_[neighbourOfNode1Index];
                    
                    if (neighbourOfNode1.size_>0)
                    {
                        if (neighbourOfNode1Index!=index0)
                        {
                            bool neighbourOfNode1AlreadyNeighbourOfNode0=false;
                            
                            for (const int32_t neighbourOfNode0Index : node0.neighbourIndices_)
                            {
                                if (nodeVect_[neighbourOfNode0Index].size_>0)
                                {
                                    if (neighbourOfNode0Index==neighbourOfNode1Index)
                                    {//Node0 and node1 already neighbours.
                                        neighbourOfNode1AlreadyNeighbourOfNode0=true;
                                        break;
                                    }
                                }
                            }
                            
                            if (!neighbourOfNode1AlreadyNeighbourOfNode0)
                            {
                                //node0.neighbourIndices_.push_back(neighbourOfNode1Index);
                                neighbourIndicesToAdd_.push_back(neighbourOfNode1Index);
                            }
                            
                            std::vector<int32_t>::iterator it=neighbourOfNode1.neighbourIndices_.begin();
                            while ( it != neighbourOfNode1.neighbourIndices_.end() )
                            {
                                int32_t & neighbourOfNeighbourOfNode1Index=*it;
                                
                                if (neighbourOfNeighbourOfNode1Index==index1)
                                {
                                    if (neighbourOfNode1AlreadyNeighbourOfNode0)
                                    {//Remove neighbourOfNode1's neighbour.
                                        neighbourOfNode1.neighbourIndices_.erase(it);
                                    } else
                                    {//Update neighbourOfNode1's neighbour.
                                        neighbourOfNeighbourOfNode1Index=index0;
                                        ++it;
                                    }
                                } else
                                {
                                    ++it;
                                }
                            }
                        }
                    }
                }
                
                node1.size_=0;
                index1=-1;
                ++numPulsesMerged;
            }
        }
    }
    
    //Remove all neighbour index entries of -1.
    std::vector<int32_t>::iterator it=node0.neighbourIndices_.begin();
    while (it!=node0.neighbourIndices_.end())
    {
        if ((*it) == -1)
        {
            node0.neighbourIndices_.erase(it);
        } else
        {
            ++it;
        }
    }
    
    node0.neighbourIndices_.insert(node0.neighbourIndices_.end(), neighbourIndicesToAdd_.begin(), neighbourIndicesToAdd_.end());
    
    return numPulsesMerged;
}
#endif

size_t FIPDPT::mergeFromAll()
{
    //=== Merge the nodes until no more merging possible ===//
    size_t numPulsesMerged=0;
    
    bool busyMerging=true;
    while (busyMerging)
    {
        busyMerging=false;
#ifdef RUN_ARCS
        const size_t numArcs=arcVect_.size();
        
        for (size_t arcIndex=0; arcIndex<numArcs; ++arcIndex)
        {
            numPulsesMerged+=mergeArc(arcIndex);
        }
#else
        //Don't use the potential list here. It might not be updated.
        for (const auto & node : nodeVect_)
        {
            if (node.size_>0)
            {
                numPulsesMerged+=mergeNode(node.index_);
            }
        }
#endif
    }
    //=== ===//
    
    return numPulsesMerged;
}

size_t FIPDPT::mergeFromList(const std::vector<int32_t> nodeIndicesToMerge)
{
    size_t numPulsesMerged=0;
    
    for (const int32_t nodeIndex : nodeIndicesToMerge)
    {
        Node &node=nodeVect_[nodeIndex];
        
#ifdef RUN_ARCS
        for (auto arcIndexNum=0; arcIndexNum<node.arcIndices_.size(); ++arcIndexNum)
        {
            numPulsesMerged+=mergeArc(node.arcIndices_[arcIndexNum]);
        }
#else
        if (node.size_>0)
        {
            numPulsesMerged+=mergeNode(nodeIndex);
        }
#endif
    }
    
    return numPulsesMerged;
}

bool FIPDPT::trigger()
{
    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {//There are images to consume and the downstream producer has space to produce.
        std::vector<Image**> imvRead=reserveReadSlot();
        std::vector<Image**> imvWrite=reserveWriteSlot();
        
        //Start stats measurement event.
        ProcessorStats_->tick();
        
        for (size_t imgNum=0; imgNum<1; ++imgNum)//For now, only process one image in each slot.
        {
            Image const * const imRead = *(imvRead[imgNum]);
            Image * const imWrite = *(imvWrite[imgNum]);
            
            uint8_t const * const dataRead=imRead->data();
            uint8_t * const dataWrite=imWrite->data();
            
            const ImageFormat imFormat=getDownstreamFormat(imgNum);
            
            const size_t width=imFormat.getWidth();
            const size_t height=imFormat.getHeight();
            
            memset(dataWrite, 0, width*height);//Clear the downstream image.
            
            
            //=== Setup the initial nodes ===//
            for (size_t y=0; y<height; ++y)
            {
                const size_t lineOffset=y * width;
                
                for (size_t x=0; x<width; ++x)
                {
                    const auto writeValue=dataRead[lineOffset + x];
                    
                    Node &node=nodeVect_[lineOffset + x];
                    node.index_=lineOffset + x;
                    node.value_=writeValue;
                    node.size_=1;
                    
#ifdef RUN_ARCS
                    node.arcIndices_.clear();
#else
                    node.neighbourIndices_.clear();
#endif
                    
                    node.pixelIndices_.clear();
                    node.pixelIndices_.push_back(lineOffset + x);
                }
            }
            //===  ==//
            
            
            //=== Setup the initial arcs or node neighbours ===//
            {
#ifdef RUN_ARCS
                size_t arcIndex=0;
#endif
                
                for (size_t y=0; y<height; ++y)
                {
                    const size_t lineOffset=y * width;
                    
                    for (size_t x=1; x<width; ++x)
                    {
#ifdef RUN_ARCS
                        Arc &arc=arcVect_[arcIndex];
                        arc.index_=arcIndex;
                        arc.active_=true;
                        
                        arc.nodeIndices_[0]=lineOffset+x-1;
                        arc.nodeIndices_[1]=lineOffset+x;
                        
                        nodeVect_[arc.nodeIndices_[0]].arcIndices_.push_back(arcIndex);
                        nodeVect_[arc.nodeIndices_[1]].arcIndices_.push_back(arcIndex);
                        arcIndex++;
#else
                        nodeVect_[lineOffset+x-1].neighbourIndices_.push_back(lineOffset+x);
                        nodeVect_[lineOffset+x].neighbourIndices_.push_back(lineOffset+x-1);
#endif
                    }
                    
                    if (y<(height-1))
                    {
                        for (size_t x=0; x<width; ++x)
                        {
#ifdef RUN_ARCS
                            Arc &arc=arcVect_[arcIndex];
                            arc.index_=arcIndex;
                            arc.active_=true;
                            
                            arc.nodeIndices_[0]=lineOffset+x;
                            arc.nodeIndices_[1]=lineOffset+x+width;
                            
                            nodeVect_[arc.nodeIndices_[0]].arcIndices_.push_back(arcIndex);
                            nodeVect_[arc.nodeIndices_[1]].arcIndices_.push_back(arcIndex);
                            arcIndex++;
#else
                            nodeVect_[lineOffset+x].neighbourIndices_.push_back(lineOffset+x+width);
                            nodeVect_[lineOffset+x+width].neighbourIndices_.push_back(lineOffset+x);
#endif
                        }
                    }
                }
            }
            //=== ===
            
            size_t policyCounter=0;
            
            size_t numBumpsRemoved=0;
            size_t numPitsRemoved=0;
            
            size_t numPulsesMerged=mergeFromAll();//Initial merge.
            
            updatePotentiallyActiveNodeIndexVect();
            
            std::vector<int32_t> nodeIndicesToMerge;
            
            size_t loopCounter = 0;
            int32_t previousSmallestPulse = 0;
            
            while ((previousSmallestPulse<filterPulseSize_)&&(potentiallyActiveNodeIndexVect_.size()>1))
            {
                nodeIndicesToMerge.clear();
                
                //=== Find smallest pulse ===//
                int32_t smallestPulse=0;
                
                for (const auto & potentiallyActiveNodeIndex : potentiallyActiveNodeIndexVect_)
                {
                    Node &node=nodeVect_[potentiallyActiveNodeIndex];
                    
                    if (node.size_>0)
                    {
                        if ((smallestPulse==0)||(node.size_<smallestPulse))
                        {
                            //if (isBump(node.index_)||isPit(node.index_))
                            //if (node.size_>previousSmallestPulse)
                            {
                                smallestPulse=node.size_;
                            }
                        }
                    }
                }
                //std::cout << "Smallest pulse is " << smallestPulse << ".\n";
                //std::cout.flush();
                //=== ===//
                
                
                //=== Implement DPT pit/bump policy ===//
                if (previousSmallestPulse!=smallestPulse)
                {
                    policyCounter=0;
                }
                //=== ===//
                
                
                //=== Remove pits and bumps according to policy ===//
                for (const auto & potentiallyActiveNodeIndex : potentiallyActiveNodeIndexVect_)
                {
                    Node &node=nodeVect_[potentiallyActiveNodeIndex];
                    
                    if ((node.size_>0)&&(node.size_<=smallestPulse))
                    {
                        if ((policyCounter%2)==0)
                        {
                            //if (isPit(node.index_))
                            {
                                //=== Remove pits ===//
                                flattenToNearestNeighbour(node.index_);
                                //flattenToFirstNeighbour(node.index_);
                                nodeIndicesToMerge.push_back(node.index_);
                                ++numPitsRemoved;
                            }
                        } else
                        {
                            //if (isBump(node.index_))
                            {
                                //=== Remove bumps ===//
                                flattenToNearestNeighbour(node.index_);
                                //flattenToFirstNeighbour(node.index_);
                                nodeIndicesToMerge.push_back(node.index_);
                                ++numBumpsRemoved;
                            }
                        }
                    }
                }
                //=== ===//
                
                
                //std::cout << "Pulses merged = " << numPulsesMerged << ".\n";
                //std::cout << "Pits removed = " << numPitsRemoved << ".\n";
                //std::cout << "Bumps removed = " << numBumpsRemoved << ".\n";
                //std::cout << "\n";
                //std::cout.flush();
                
                
                //=== Merge over arcs of removed nodes ===//
                numPulsesMerged=mergeFromList(nodeIndicesToMerge);
                //=== ===//
                
                ++policyCounter;
                previousSmallestPulse=smallestPulse;
                
                if ((loopCounter&15)==0) updatePotentiallyActiveNodeIndexVect();
                ++loopCounter;
            }
            
            
            //=============================================//
            //=============================================//
            //=============================================//
            
            
            {
                //=== Find smallest pulse ==
                int32_t smallestPulse=0;
                for (const auto & node : nodeVect_)
                {
                    if (node.size_>0)
                    {
                        if ((smallestPulse==0)||(node.size_<smallestPulse))
                        {
                            //if (isBump(node.index_)||isPit(node.index_))
                            {
                                smallestPulse=node.size_;
                            }
                        }
                    }
                }
                //=== ===
                
                //std::cout << "Drawing pulses of size " << smallestPulse << "+.\n";
                //std::cout.flush();
                
                //=== Draw the pulses on the sceen ==
                for (const auto & node : nodeVect_)
                {
                    if (node.size_>0)
                    {
                        //if (node.size_==smallestPulse)
                        {
                            for (const auto pixelIndex : node.pixelIndices_)
                            {
                                dataWrite[pixelIndex]=node.value_;
                            }
                        }
                    }
                }
                
                //std::cout << "\n";
                //std::cout.flush();
                //=== ===
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

