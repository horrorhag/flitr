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

#ifndef FIP_DPT_H
#define FIP_DPT_H 1

#include <flitr/image_processor.h>

namespace flitr {
    
    #define RUN_ARCS 1
    
    struct Node
    {
    public:
        int32_t index_;
        uint8_t value_;
        int32_t size_;
        
#ifdef RUN_ARCS
        std::vector<int32_t> arcIndices_;
#else
        std::vector<int32_t> neighbourIndices_;
#endif
        
        std::vector<int32_t> pixelIndices_;
    };
    
#ifdef RUN_ARCS
    struct Arc
    {
    public:
        int32_t index_;
        bool active_;
        int32_t nodeIndices_[2];
    };
#endif
    
    /*! Compute the DPT of image. Currently assumes 8-bit mono input! */
    class FLITR_EXPORT FIPDPT : public ImageProcessor
    {
    public:
        
        /*! Constructor given the upstream producer.
         *@param upStreamProducer The upstream image producer.
         *@param images_per_slot The number of images per image slot from the upstream producer.
         *@param buffer_size The size of the shared image buffer of the downstream producer.*/
        FIPDPT(ImageProducer& upStreamProducer, int32_t filterPulseSize,
               uint32_t images_per_slot,
               uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);
        
        /*! Virtual destructor */
        virtual ~FIPDPT();
        
        /*! Method to initialise the object.
         *@return Boolean result flag. True indicates successful initialisation.*/
        virtual bool init();
        
        /*!Synchronous trigger method. Called automatically by the trigger thread if started.
         *@sa ImageProcessor::startTriggerThread*/
        virtual bool trigger();
        
    private:
        
#ifdef RUN_ARCS
        inline int32_t retrieveNeighbourNodeIndex(const Arc &arc, const int32_t nodeIndex)
        {
            return (arc.nodeIndices_[0]==nodeIndex) ? arc.nodeIndices_[1] : arc.nodeIndices_[0];
        }
#endif
        
        inline bool isBump(const size_t nodeIndex)
        {
            const Node &node=nodeVect_[nodeIndex];
            
#ifdef RUN_ARCS
            for (const auto arcIndex : node.arcIndices_)
            {
                const auto &arc=arcVect_[arcIndex];
                
                if (arc.active_)
                {
                    const auto neighbourIndex=retrieveNeighbourNodeIndex(arc, nodeIndex);
                    
                    if (nodeVect_[neighbourIndex].value_ >= node.value_)
                    {
                        return false;
                    }
                }
            }
#else
            for (const auto neighbourIndex : node.neighbourIndices_)
            {
                if (nodeVect_[neighbourIndex].size_)
                {
                    if (nodeVect_[neighbourIndex].value_ >= node.value_)
                    {
                        return false;
                    }
                }
            }
#endif
            return true;
        }
        
        inline bool isPit(const size_t nodeIndex)
        {
            const Node &node=nodeVect_[nodeIndex];
            
#ifdef RUN_ARCS
            for (const auto arcIndex : node.arcIndices_)
            {
                const auto &arc=arcVect_[arcIndex];
                
                if (arc.active_)
                {
                    const auto neighbourIndex=retrieveNeighbourNodeIndex(arc, nodeIndex);
                    
                    if (nodeVect_[neighbourIndex].value_ <= node.value_)
                    {
                        return false;
                    }
                }
            }
#else
            for (const auto neighbourIndex : node.neighbourIndices_)
            {
                if (nodeVect_[neighbourIndex].size_)
                {
                    if (nodeVect_[neighbourIndex].value_ <= node.value_)
                    {
                        return false;
                    }
                }
            }
#endif
            
            return true;
        }
        
        int16_t flattenToFirstNeighbour(const size_t nodeIndex)
        {
            Node &node=nodeVect_[nodeIndex];
            
#ifdef RUN_ARCS
            size_t firstNeighbourIndex=nodeIndex;
            
            for (const auto arcIndex : node.arcIndices_)
            {
                const auto &arc=arcVect_[arcIndex];
                
                if (arc.active_)
                {
                    firstNeighbourIndex=retrieveNeighbourNodeIndex(arc, nodeIndex);
                    break;
                }
            }
#else
            size_t firstNeighbourIndex=nodeIndex;
            
            for (const auto neighbourIndex : node.neighbourIndices_)
            {
                if (nodeVect_[neighbourIndex].size_>0)
                {
                    firstNeighbourIndex=neighbourIndex;
                    break;
                }
            }
#endif
            
            node.value_=nodeVect_[firstNeighbourIndex].value_;
            
            return node.value_;
        }
        
        uint8_t flattenToNearestNeighbour(const size_t nodeIndex)
        {
            Node &node=nodeVect_[nodeIndex];
            
            size_t nearestNeighbourIndex=nodeIndex;
            
#ifdef RUN_ARCS
            for (const auto arcIndex : node.arcIndices_)
            {
                const auto &arc=arcVect_[arcIndex];
                
                if (arc.active_)
                {
                    const auto neighbourIndex=retrieveNeighbourNodeIndex(arc, nodeIndex);
                    
                    if (( nearestNeighbourIndex==nodeIndex ) ||
                        ( abs(int(nodeVect_[neighbourIndex].value_)-int(node.value_)) <= abs(int(nodeVect_[nearestNeighbourIndex].value_)-int(node.value_)) ))
                    {
                        nearestNeighbourIndex=neighbourIndex;
                    }
                }
            }
#else
            for (const auto neighbourIndex : node.neighbourIndices_)
            {
                if (nodeVect_[neighbourIndex].size_>0)
                {
                    if (( nearestNeighbourIndex==nodeIndex ) ||
                        ( abs(int(nodeVect_[neighbourIndex].value_)-int(node.value_)) <= abs(int(nodeVect_[nearestNeighbourIndex].value_)-int(node.value_)) ))
                    {
                        nearestNeighbourIndex=neighbourIndex;
                    }
                }
            }
#endif
            
            node.value_=nodeVect_[nearestNeighbourIndex].value_;
            
            return node.value_;
        }
        
        void updatePotentiallyActiveNodeIndexVect()
        {
            potentiallyActiveNodeIndexVect_.clear();
            
            for (const auto & node : nodeVect_)
            {
                if (node.size_>0)
                {
                    potentiallyActiveNodeIndexVect_.push_back(node.index_);
                }
            }
        }
        
#ifdef RUN_ARCS
        size_t mergeArc(const int32_t arcIndex);
#else
        size_t mergeNode(const int32_t nodeIndex);
#endif
        
        size_t mergeFromAll();
        size_t mergeFromList(const std::vector<int32_t> nodeIndicesToMerge);
        
        
    private:
        int32_t filterPulseSize_;
        
        std::vector<Node> nodeVect_;
        std::vector<size_t> potentiallyActiveNodeIndexVect_;
        
#ifdef RUN_ARCS
        std::vector<Arc> arcVect_;
#else
        std::vector<int32_t> neighbourIndicesToAdd_;
#endif
    };
    
}

#endif //FIP_DPT_H
