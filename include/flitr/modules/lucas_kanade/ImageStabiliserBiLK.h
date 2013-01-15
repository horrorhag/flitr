#ifndef IMAGE_STABALISER_BILK
#define IMAGE_STABALISER_BILK

#define NEWTON_RAPHSON_ITERATIONS 7

#include <iostream>
#include <boost/tr1/memory.hpp>

using std::tr1::shared_ptr;

#include <flitr/image_format.h>
#include <flitr/image.h>
#include <flitr/modules/lucas_kanade/postRenderCallback.h>
#include <flitr/modules/lucas_kanade/imageBiPyramid.h>
#include <osg/MatrixTransform>


#define maxBLK(a,b)	(((a) > (b)) ? (a) : (b))
#define minBLK(a,b)	(((a) < (b)) ? (a) : (b))


namespace flitr
{
class ImageStabiliserBiLK
{
public:
    static double logbase(double a, double base);


    class PostBiLKIteration_CameraPostDrawCallback : public osg::Camera::DrawCallback
    {//Callback signalling when a GPU BiLK iteration has completed and been readback to the CPU.
    public:
        PostBiLKIteration_CameraPostDrawCallback(ImageStabiliserBiLK *i_pImageStabiliserBiLK) :
            m_pImageStabiliserBiLK(i_pImageStabiliserBiLK)
        {}

        virtual void operator () (const osg::Camera& camera) const
        {
            //					std::cout << "PostLKIterationCallback\n";
            //					std::cout.flush();

            m_pImageStabiliserBiLK->m_ulNumLKIterations++;

            //Calculate the LK Result's resolution/pyramid level from 'm_ulNumLKIterations' counter.
            long lKResultPyrLevel=m_pImageStabiliserBiLK->m_pCurrentBiPyramid->getNumLevels()-
                    (m_pImageStabiliserBiLK->m_ulNumLKIterations-1)/NEWTON_RAPHSON_ITERATIONS-1;


            const osg::Image *lKResultImage_left=(m_pImageStabiliserBiLK->m_ulNumGPUHVectorReductionLevels>0) ?
                        m_pImageStabiliserBiLK->getLKReducedResultLevel(0, minBLK(lKResultPyrLevel+m_pImageStabiliserBiLK->m_ulNumGPUHVectorReductionLevels, m_pImageStabiliserBiLK->m_ulMaxGPUHVectorReductionLevels-1)) :
                        m_pImageStabiliserBiLK->getLKResultLevel(0, lKResultPyrLevel);

            const osg::Image *lKResultImage_right=(m_pImageStabiliserBiLK->m_ulNumGPUHVectorReductionLevels>0) ?
                        m_pImageStabiliserBiLK->getLKReducedResultLevel(1, minBLK(lKResultPyrLevel+m_pImageStabiliserBiLK->m_ulNumGPUHVectorReductionLevels, m_pImageStabiliserBiLK->m_ulMaxGPUHVectorReductionLevels-1)) :
                        m_pImageStabiliserBiLK->getLKResultLevel(1, lKResultPyrLevel);

            const unsigned long lkLevelBorder=(m_pImageStabiliserBiLK->m_ulNumGPUHVectorReductionLevels>0) ?
                        0 :
                        (flitr::ImageStabiliserBiLK::m_ulLKBorder>>lKResultPyrLevel);


            long lKResultWidth=lKResultImage_left->s();
            long lKResultHeight=lKResultImage_left->t();

            osg::Vec2d hNumerator_left=osg::Vec2d(0.0, 0.0);
            osg::Vec2 hEstimate_left=osg::Vec2(0.0, 0.0);
            float hDenominator_left=0.0;

            osg::Vec2d hNumerator_right=osg::Vec2d(0.0, 0.0);
            osg::Vec2 hEstimate_right=osg::Vec2(0.0, 0.0);
            float hDenominator_right=0.0;

            m_pImageStabiliserBiLK->m_rpHEstimateUniform_left->get(hEstimate_left);
            m_pImageStabiliserBiLK->m_rpHEstimateUniform_right->get(hEstimate_right);

            if (((m_pImageStabiliserBiLK->m_ulNumLKIterations-1)%NEWTON_RAPHSON_ITERATIONS)==0)
            {//New level which is at twice the resolution of the previous level.
                //hEstimate not multiplied by two for consecutive iterations.
                hEstimate_left*=2.0;
                hEstimate_right*=2.0;
            }


            //============================
            //==== Calculate h-vector ====
            //============================
            {//Note: CPU reduction ONLY. Sum lKResultImage!
                //      Ignore border of LUCAS_KANADE_BORDER pixels to improve stability.
                float *data_left=(float *)lKResultImage_left->data();
                float *data_right=(float *)lKResultImage_right->data();

                data_left+=4*lKResultWidth * lkLevelBorder;
                data_right+=4*lKResultWidth * lkLevelBorder;
                for (long y=(lKResultHeight-lkLevelBorder); y>(long)lkLevelBorder; y--)
                {
                    data_left+=4 * lkLevelBorder;
                    data_right+=4 * lkLevelBorder;
                    for (long x=(lKResultWidth-lkLevelBorder); x>(long)lkLevelBorder; x--)
                    {
                        hNumerator_left+=osg::Vec2(*(data_left+0), *(data_left+1));
                        hDenominator_left+=*(data_left+2);
                        data_left+=4;

                        hNumerator_right+=osg::Vec2(*(data_right+0), *(data_right+1));
                        hDenominator_right+=*(data_right+2);
                        data_right+=4;
                    }
                    data_left+=4 * lkLevelBorder;
                    data_right+=4 * lkLevelBorder;
                }
            }

            osg::Vec2d deltaHEstimate_left=(hDenominator_left!=0.0) ? (-hNumerator_left/hDenominator_left) : osg::Vec2d(0.0, 0.0);
            osg::Vec2d deltaHEstimate_right=(hDenominator_right!=0.0) ? (-hNumerator_right/hDenominator_right) : osg::Vec2d(0.0, 0.0);
            //============================
            //============================
            //============================

            hEstimate_left+=deltaHEstimate_left;
            hEstimate_right+=deltaHEstimate_right;

            m_pImageStabiliserBiLK->m_rpHEstimateUniform_left->set(hEstimate_left);
            m_pImageStabiliserBiLK->m_rpHEstimateUniform_right->set(hEstimate_right);


            if (m_pImageStabiliserBiLK->m_ulNumLKIterations>=(m_pImageStabiliserBiLK->m_pCurrentBiPyramid->getNumLevels()*NEWTON_RAPHSON_ITERATIONS))
            {//LK is done!
                m_pImageStabiliserBiLK->m_ulNumLKIterations=0;

                m_pImageStabiliserBiLK->m_h_left=hEstimate_left;
                m_pImageStabiliserBiLK->m_h_right=hEstimate_right;

                if (m_pImageStabiliserBiLK->m_ulFrameNumber>=2)
                {
                    osg::Matrixd deltaTransform=m_pImageStabiliserBiLK->getDeltaTransformationMatrix();

                    for (int i=0; i<(int)(m_pImageStabiliserBiLK->filterPairs_.size()); i++)
                    {

                        m_pImageStabiliserBiLK->filterPairs_[i].first=m_pImageStabiliserBiLK->filterPairs_[i].first*(1.0f-m_pImageStabiliserBiLK->filterPairs_[i].second)+osg::Vec2d(deltaTransform(3,0), deltaTransform(3,2))*(m_pImageStabiliserBiLK->filterPairs_[i].second);

                        if (i==0)
                        {//Update output quad's transform with the delta transform.
                            osg::Matrixd quadDeltaTransform=deltaTransform;
                            quadDeltaTransform(3,0)=quadDeltaTransform(3,0)-m_pImageStabiliserBiLK->filterPairs_[i].first._v[0];
                            quadDeltaTransform(3,2)=quadDeltaTransform(3,2)-m_pImageStabiliserBiLK->filterPairs_[i].first._v[1];

                            osg::Matrixd quadTransform=m_pImageStabiliserBiLK->m_rpQuadMatrixTransform->getMatrix();
                            quadTransform=quadDeltaTransform*quadTransform;
                            m_pImageStabiliserBiLK->m_rpQuadMatrixTransform->setMatrix(quadTransform);
                            m_pImageStabiliserBiLK->offsetQuadPositionByMatrix(&quadTransform,
                                                                               m_pImageStabiliserBiLK->m_pInputTexture->getTextureWidth(),
                                                                               m_pImageStabiliserBiLK->m_pInputTexture->getTextureHeight());
                        }
                    }
                }

            }
        }

    private:
        ImageStabiliserBiLK *m_pImageStabiliserBiLK;
    };
    //==========================================================================
    //===================================================
    //Class: PostBiPyramidRebuiltCallback
    // Use case 1 - Called from imageBiPyramid every time the Group Node created by createDerivLevel(...)
    //              has been rendered. This callback's pointer is passed as a parameter to createDerivLevel(...)!
    //              If the pointer passed to createDerivLevel(...) is zero then the callback will not be called.
    //             * The PostPyramidRebuiltCallback may be used if the Image pyramid is built on the GPU, but the
    //               next step of the algo is done on the CPU. Not required if the next step is done on the GPU.
    //===================================================
    class PostBiPyramidRebuiltCallback : public PostRenderCallback
    {//Used if Pyramid built on GPU, but rest of algorithm run on CPU.
    public:
        PostBiPyramidRebuiltCallback(ImageStabiliserBiLK *i_pImageStabaliserBiLK) :
            m_pImageStabiliserBiLK(i_pImageStabaliserBiLK)
        {}

        virtual void callback();

    private:
        ImageStabiliserBiLK *m_pImageStabiliserBiLK;
    };
    //==========================================================================



    ImageStabiliserBiLK(const osg::TextureRectangle *i_pInputTexture,
                        unsigned long i_ulROIX_left, unsigned long i_ulROIY_left,
                        unsigned long i_ulROIX_right, unsigned long i_ulROIY_right,
                        unsigned long i_ulROIWidth, unsigned long i_ulROIHeight,
                        bool i_bIndicateROI,
                        bool i_bDoGPUPyramids,
                        bool i_bDoGPULKIterations,
                        unsigned long i_ulNumGPUHVectorReductionLevels,
                        bool i_bReadOutputBackToCPU,
                        bool i_bBiLinearOutputFilter,
                        int i_iOutputScaleFactor,
                        float i_fOutputCropFactor,//larger than one to crop, smaller than one to frame.
                        float filterHistory=0.0f,
                        int numFilterPairs=1);


    bool init(osg::Group *root_group);

    inline const osg::Image* getLKResultLevel(unsigned long i_ulPyramidNum, unsigned long i_ulLevel) const
    {
        return m_lkResultImagePyramid[i_ulPyramidNum][i_ulLevel].get();
    }
    inline const osg::Image* getLKReducedResultLevel(unsigned long i_ulPyramidNum, unsigned long i_ulLevel) const
    {
        return m_lkReducedResultImagePyramid[i_ulPyramidNum][i_ulLevel].get();
    }




    inline void setAutoSwapCurrentPrevious(bool i_bValue)
    {
        m_bAutoSwapCurrentPrevious=i_bValue;
    }

    //Note: MUST be called before triggerInput()!!!
    inline void swapCurrentPrevious()
    {
        //Swap current and previous ImageBiPyramid node pointers AND switch_on node pointed to by 'current'.
        ImageBiPyramid *tempBiPyramid=m_pCurrentBiPyramid;
        m_pCurrentBiPyramid=m_pPreviousBiPyramid;
        m_pPreviousBiPyramid=tempBiPyramid;

        if (m_bDoGPULKIterations)
        {//Swap current and previous LKIteration node pointers AND switch_on node pointed to by 'current'.
            osg::ref_ptr<osg::Switch> tempRefPtrSwitch=m_rpCurrentLKIterationRebuildSwitch;
            m_rpCurrentLKIterationRebuildSwitch=m_rpPreviousLKIterationRebuildSwitch;
            m_rpPreviousLKIterationRebuildSwitch=tempRefPtrSwitch;
        }

        m_ulFrameNumber++;
    }

    inline void triggerInput()
    {
        if (m_bAutoSwapCurrentPrevious)
        {
            swapCurrentPrevious();
        }

        m_h_left=osg::Vec2(0.0, 0.0);
        m_h_right=osg::Vec2(0.0, 0.0);

        m_rpHEstimateUniform_left->set(m_h_left);
        m_rpHEstimateUniform_right->set(m_h_right);

        if (m_bDoGPUPyramids)
        {
            m_pCurrentBiPyramid->setRebuildSwitchOn();
            m_pPreviousBiPyramid->setRebuildSwitchOff();
        } else
        {
            m_pCurrentBiPyramid->rebuildBiPyramidCPU();
            m_postBiPyramidRebuiltCallback.callback();
        }

        if (m_bDoGPULKIterations)
        {
            m_rpCurrentLKIterationRebuildSwitch->setAllChildrenOn();
            m_rpPreviousLKIterationRebuildSwitch->setAllChildrenOff();
        }
    }

    void updateH_BiLucasKanadeCPU();

    inline osg::TextureRectangle *getOutputTexture() const
    {
        return m_outputTexture.get();
    }

    inline void resetQuadMatrixTransform(const osg::Matrixd &i_rMatrix)
    {
        m_rpQuadMatrixTransform->setMatrix(i_rMatrix);
        filterPairs_[0].first=osg::Vec2f(i_rMatrix(3,0), i_rMatrix(3,2));
    }

    osg::Matrixd getDeltaTransformationMatrix() const;

    void getHomographyMatrix(double &a00, double &a01, double &a02,
                             double &a10, double &a11, double &a12,
                             double &a20, double &a21, double &a22) const;

    inline osg::Vec2 getLeftHVector() const
    {
        return m_h_left;
    }

    inline osg::Vec2 getRightHVector() const
    {
        return m_h_right;
    }


    void setFilterHistory(float value, uint16_t filterNum)
    {
        if (filterNum<filterPairs_.size())
        {
            filterPairs_[filterNum].second=value;
        }
    }

    osg::Vec2f getFilteredSpeed(uint16_t filterNum) const
    {
        if (filterNum<filterPairs_.size())
        {
            return filterPairs_[filterNum].first;
        }

        return osg::Vec2f(0.0f, 0.0f);
    }

    void  setFilteredSpeed(osg::Vec2f speed, uint16_t filterNum)
    {
        if (filterNum<filterPairs_.size())
        {
            filterPairs_[filterNum].first=speed;
        }
    }

public:
    const unsigned long m_ulROIX_left, m_ulROIY_left;
    const unsigned long m_ulROIX_right, m_ulROIY_right;
    const unsigned long m_ulROIWidth, m_ulROIHeight;

    unsigned long m_ulNumLKIterations;

    ImageBiPyramid *m_pCurrentBiPyramid;
    ImageBiPyramid *m_pPreviousBiPyramid;

private:
    bool m_bAutoSwapCurrentPrevious;

    osg::ref_ptr<osg::MatrixTransform> m_rpQuadMatrixTransform;
    osg::ref_ptr<osg::Geometry>       m_quadGeom;

    osg::ref_ptr<osg::TextureRectangle> m_outputTexture;
    osg::ref_ptr<osg::Image> m_outputOSGImage;
    bool m_bReadOutputBackToCPU;
    int m_iOutputScaleFactor;
    float m_fOutputCropFactor;

    PostBiPyramidRebuiltCallback m_postBiPyramidRebuiltCallback;

    PostBiLKIteration_CameraPostDrawCallback m_postBiLKIterationDrawCallback;

    osg::ref_ptr<osg::Image> **m_lkResultImagePyramid;
    osg::ref_ptr<osg::TextureRectangle> **m_lkResultTexturePyramid;
    osg::ref_ptr<osg::Image> **m_lkReducedResultImagePyramid;
    osg::ref_ptr<osg::TextureRectangle> **m_lkReducedResultTexturePyramid;

    ImageFormat *m_imagePyramidFormat_;
    osg::ref_ptr<osg::Switch> m_rpCurrentLKIterationRebuildSwitch;
    osg::ref_ptr<osg::Switch> m_rpPreviousLKIterationRebuildSwitch;
    osg::ref_ptr<osg::Uniform> m_rpHEstimateUniform_left;
    osg::ref_ptr<osg::Uniform> m_rpHEstimateUniform_right;


    osg::Node* createHVectorReductionLevel(unsigned long i_ulLevel, unsigned long i_ulWidth, unsigned long i_ulHeight, unsigned long i_ulReductionLevel, bool i_bDoPostLKIterationCallback);
    osg::Node* createBiLKIteration(ImageBiPyramid *i_pCurrentBiPyramid, ImageBiPyramid *i_pPreviousBiPyramid, unsigned long i_ulLevel, unsigned long i_ulWidth, unsigned long i_ulHeight, osg::Uniform *i_pHEstimateUniform_left, osg::Uniform *i_pHEstimateUniform_right, bool i_bDoPostLKIterationCallback);
    osg::ref_ptr<osg::Geode> createScreenAlignedQuadLK(unsigned long i_ulWidth, unsigned long i_ulHeight, double i_dTextXOffset, double i_dTextYOffset, double i_dBorderPixels=0);
    osg::Camera *createScreenAlignedCameraLK(unsigned long i_ulWidth, unsigned long i_ulHeight, double i_dBorderPixels=0.0);

    const osg::TextureRectangle *m_pInputTexture;

    const bool m_bIndicateROI;
    bool m_bDoGPUPyramids;
    const bool m_bDoGPULKIterations;
    unsigned long m_ulNumGPUHVectorReductionLevels;
    unsigned long m_ulMaxGPUHVectorReductionLevels;

    bool m_bBiLinearOutputFilter;

    osg::Vec2 m_h_left;
    osg::Vec2 m_h_right;
    unsigned long m_ulFrameNumber;

    void offsetQuadPositionByMatrix(const osg::Matrixd *i_pTransformation, unsigned long i_ulWidth, unsigned long i_ulHeight);

    osg::ref_ptr<osg::Geode> createScreenAlignedQuad(unsigned long i_ulWidth, unsigned long i_ulHeight, double i_dTextXOffset, double i_dTextYOffset);
    osg::Camera *createScreenAlignedCamera(unsigned long i_ulWidth, unsigned long i_ulHeight);

    osg::Node* createOutputPass();

    inline osg::Vec2f getROI_leftCentre() const
    {
        return osg::Vec2f(m_ulROIX_left+m_ulROIWidth*0.5,
                          m_ulROIY_left+m_ulROIHeight*0.5);
    }
    inline osg::Vec2f getROI_rightCentre() const
    {
        return osg::Vec2f(m_ulROIX_right+m_ulROIWidth*0.5,
                          m_ulROIY_right+m_ulROIHeight*0.5);
    }


    static unsigned long m_ulLKBorder;

    std::vector<std::pair<osg::Vec2f, float> > filterPairs_;//Filtered speed AND filter history factor.
};
}
#endif
