#ifndef IMAGE_STABALISER_NLK
#define IMAGE_STABALISER_NLK

#ifndef NEWTON_RAPHSON_ITERATIONS
#define NEWTON_RAPHSON_ITERATIONS 7
#endif

#include <iostream>
#include <boost/tr1/memory.hpp>

using std::tr1::shared_ptr;

#include <flitr/flitr_export.h>
#include <flitr/image_format.h>
#include <flitr/image.h>
#include <flitr/modules/lucas_kanade/postRenderCallback.h>
#include <flitr/modules/lucas_kanade/ImageMultiPyramid.h>
#include <osg/MatrixTransform>


namespace flitr
{

#define NUM_OUTPUT_QUAD_STEPS 10

class FLITR_EXPORT ImageStabiliserMultiLK
{
public:
    static double logbase(const double a, const double base);


    class PostNLKIteration_CameraPostDrawCallback : public osg::Camera::DrawCallback
    {//Callback signalling when a GPU LK iteration has completed that should reduced by the CPU.
        //Will be called at least once after last GPU hvector reduction.
    public:
        PostNLKIteration_CameraPostDrawCallback(ImageStabiliserMultiLK *i_pImageStabiliserNLK) :
            m_pImageStabiliserNLK(i_pImageStabiliserNLK)
        {}

        virtual void operator () (const osg::Camera& camera) const
        {
            //					std::cout << "PostLKIterationCallback\n";
            //					std::cout.flush();

            m_pImageStabiliserNLK->m_ulNumLKIterations++;

            //Calculate the LK Result's resolution/pyramid level from 'm_ulNumLKIterations' counter.
            long lKResultPyrLevel=m_pImageStabiliserNLK->m_pCurrentNPyramid->getNumLevels()-
                    (m_pImageStabiliserNLK->m_ulNumLKIterations-1)/NEWTON_RAPHSON_ITERATIONS-1;

            const unsigned long lkLevelBorder=(m_pImageStabiliserNLK->m_ulNumGPUHVectorReductionLevels>0) ?
                        0 :
                        (flitr::ImageStabiliserMultiLK::m_ulLKBorder>>lKResultPyrLevel);

            for (int i=0; i<(int)m_pImageStabiliserNLK->m_numPyramids_; i++)
            {
                const osg::Image *lKResultImage=(m_pImageStabiliserNLK->m_ulNumGPUHVectorReductionLevels>0) ?
                            m_pImageStabiliserNLK->getLKReducedResultLevel(i, std::min<unsigned long>(lKResultPyrLevel+m_pImageStabiliserNLK->m_ulNumGPUHVectorReductionLevels, m_pImageStabiliserNLK->m_ulMaxGPUHVectorReductionLevels-1)) :
                            m_pImageStabiliserNLK->getLKResultLevel(i, lKResultPyrLevel);

                long lKResultWidth=lKResultImage->s();
                long lKResultHeight=lKResultImage->t();

                osg::Vec2d hNumerator=osg::Vec2d(0.0, 0.0);
                osg::Vec2 hEstimate=osg::Vec2(0.0, 0.0);
                float hDenominator=0.0;

                m_pImageStabiliserNLK->m_rpHEstimateUniform[i]->get(hEstimate);

                if (((m_pImageStabiliserNLK->m_ulNumLKIterations-1)%NEWTON_RAPHSON_ITERATIONS)==0)
                {//New level which is at twice the resolution of the previous level.
                    //hEstimate not multiplied by two for consecutive newton-raphson iterations at same level.
                    hEstimate*=2.0;
                }

                //==============================================
                //==== CPU Reduce: Calculate h-vector delta ====
                //==============================================
                {//Note: CPU reduction ONLY. Sum lKResultImage!
                    //      Ignore border of LUCAS_KANADE_BORDER pixels to improve stability.
                    float *data=(float *)lKResultImage->data();

                    data+=4*lKResultWidth * lkLevelBorder;
                    for (long y=(lKResultHeight-lkLevelBorder); y>(long)lkLevelBorder; y--)
                    {
                        data+=4 * lkLevelBorder;
                        for (long x=(lKResultWidth-lkLevelBorder); x>(long)lkLevelBorder; x--)
                        {
                            hNumerator+=osg::Vec2(*(data+0), *(data+1));
                            hDenominator+=*(data+2);
                            data+=4;
                        }
                        data+=4 * lkLevelBorder;
                    }
                }

                osg::Vec2d deltaHEstimate=(hDenominator!=0.0) ? (-hNumerator/hDenominator) : osg::Vec2d(0.0, 0.0);
                //==============================================
                //==============================================
                //==============================================

                hEstimate+=deltaHEstimate;

                m_pImageStabiliserNLK->m_rpHEstimateUniform[i]->set(hEstimate);

                m_pImageStabiliserNLK->m_h[i]=hEstimate;
            }



            if (m_pImageStabiliserNLK->m_ulNumLKIterations>=(m_pImageStabiliserNLK->m_pCurrentNPyramid->getNumLevels()*NEWTON_RAPHSON_ITERATIONS))
            {//LK iterations and h-vector reductions are done so update the quad transform.
                m_pImageStabiliserNLK->m_ulNumLKIterations=0;

                //==============================================
                //====== Do the quad transform update ==========
                //==============================================
                if (m_pImageStabiliserNLK->m_ulFrameNumber>=2)
                {
                    m_pImageStabiliserNLK->updateOutputQuadTransform();
                }
                //==============================================
                //==============================================
                //==============================================
            }
        }

    private:
        ImageStabiliserMultiLK *m_pImageStabiliserNLK;
    };


    //==========================================================================
    class PostNPyramidRebuiltCallback : public PostRenderCallback
    {//Called if Pyramid built on GPU, but rest of algorithm run on CPU.
    public:
        PostNPyramidRebuiltCallback(ImageStabiliserMultiLK *i_pImageStabiliserNLK) :
            m_pImageStabiliserNLK(i_pImageStabiliserNLK)
        {}

        virtual void callback();

    private:
        ImageStabiliserMultiLK *m_pImageStabiliserNLK;
    };
    //==========================================================================



    ImageStabiliserMultiLK(const flitr::TextureRectangle *i_pInputTexture,
                           std::vector< std::pair<int,int> > &i_ROIVec,
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

    uint32_t getNumPyramids() const
    {
        return m_ROIVec.size();
    }

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
        ImageNPyramid *tempNPyramid=m_pCurrentNPyramid;
        m_pCurrentNPyramid=m_pPreviousNPyramid;
        m_pPreviousNPyramid=tempNPyramid;

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

        for (int i=0; i<(int)m_numPyramids_; i++)
        {
            m_h[i]=osg::Vec2(0.0, 0.0);

            m_rpHEstimateUniform[i]->set(m_h[i]);
        }


        if (m_bDoGPUPyramids)
        {
            m_pCurrentNPyramid->setRebuildSwitchOn();
            m_pPreviousNPyramid->setRebuildSwitchOff();
        } else
        {
            m_pCurrentNPyramid->rebuildNPyramidCPU();
            m_postNPyramidRebuiltCallback.callback();
        }

        if (m_bDoGPULKIterations)
        {
            m_rpCurrentLKIterationRebuildSwitch->setAllChildrenOn();
            m_rpPreviousLKIterationRebuildSwitch->setAllChildrenOff();
        }
    }

    void updateH_NLucasKanadeCPU();

    inline flitr::TextureRectangle *getOutputTexture() const
    {
        return m_outputTexture.get();
    }

    inline void resetQuadMatrixTransform(const osg::Matrixd &i_rMatrix)
    {
        m_rpQuadMatrixTransform->setMatrix(i_rMatrix);
        filterPairs_[0].first=osg::Vec2f(i_rMatrix(3,0), i_rMatrix(3,2));
    }

    osg::Matrixd getDeltaTransformationMatrix() const;

    inline osg::Vec2 getHVector(unsigned long i_ulPyramidNum) const
    {
        return m_h[i_ulPyramidNum];
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


    inline osg::Vec2f getROI_Centre(const unsigned long i_ulPyramidNum) const
    {
        return osg::Vec2f(m_ROIVec[i_ulPyramidNum].first+m_ulROIWidth*0.5f,
                          m_ROIVec[i_ulPyramidNum].second+m_ulROIHeight*0.5f);
    }

    inline osg::Vec2f getTransformedROI_Centre(const unsigned long i_ulPyramidNum) const
    {
        return osg::Vec2f(m_TransformedROICentreVec[i_ulPyramidNum].first,
                          m_TransformedROICentreVec[i_ulPyramidNum].second);
    }

public:
    std::vector< std::pair<int,int> > m_ROIVec;
    unsigned long m_numPyramids_;

    const unsigned long m_ulROIWidth, m_ulROIHeight;

    unsigned long m_ulNumLKIterations;

    ImageNPyramid *m_pCurrentNPyramid;
    ImageNPyramid *m_pPreviousNPyramid;

private:
    bool m_bAutoSwapCurrentPrevious;

    osg::ref_ptr<osg::MatrixTransform> m_rpQuadMatrixTransform;
    std::vector< std::pair<double,double> > m_TransformedROICentreVec;
    osg::ref_ptr<osg::Geometry>       m_quadGeom;

    osg::ref_ptr<flitr::TextureRectangle> m_outputTexture;
    osg::ref_ptr<osg::Image> m_outputOSGImage;

    bool m_bReadOutputBackToCPU;
    int m_iOutputScaleFactor;
    float m_fOutputCropFactor;

    PostNPyramidRebuiltCallback m_postNPyramidRebuiltCallback;

    PostNLKIteration_CameraPostDrawCallback m_postNLKIterationDrawCallback;

    osg::ref_ptr<osg::Image> **m_lkResultImagePyramid;
    osg::ref_ptr<flitr::TextureRectangle> **m_lkResultTexturePyramid;
    osg::ref_ptr<osg::Image> **m_lkReducedResultImagePyramid;
    osg::ref_ptr<flitr::TextureRectangle> **m_lkReducedResultTexturePyramid;

    ImageFormat *m_imagePyramidFormat_;
    osg::ref_ptr<osg::Switch> m_rpCurrentLKIterationRebuildSwitch;
    osg::ref_ptr<osg::Switch> m_rpPreviousLKIterationRebuildSwitch;
    osg::ref_ptr<osg::Uniform> *m_rpHEstimateUniform;


    osg::Node* createHVectorReductionLevel(unsigned long i_ulLevel, unsigned long i_ulWidth, unsigned long i_ulHeight, unsigned long i_ulReductionLevel, bool i_bDoPostLKIterationCallback);

    osg::Node* createNLKIteration(ImageNPyramid *i_pCurrentNPyramid, ImageNPyramid *i_pPreviousNPyramid, unsigned long i_ulLevel, unsigned long i_ulWidth, unsigned long i_ulHeight, bool i_bDoPostLKIterationCallback);

    osg::ref_ptr<osg::Geode> createScreenAlignedQuadLK(unsigned long i_ulWidth, unsigned long i_ulHeight, double i_dBorderPixels=0);
    osg::Camera *createScreenAlignedCameraLK(unsigned long i_ulWidth, unsigned long i_ulHeight, double i_dBorderPixels=0.0);

    const flitr::TextureRectangle *m_pInputTexture;

    const bool m_bIndicateROI;
    bool m_bDoGPUPyramids;
    const bool m_bDoGPULKIterations;
    unsigned long m_ulNumGPUHVectorReductionLevels;
    unsigned long m_ulMaxGPUHVectorReductionLevels;

    bool m_bBiLinearOutputFilter;

    osg::Vec2 *m_h;
    unsigned long m_ulFrameNumber;


private:
    osg::Vec2f transformLRCoordByROICentres(const osg::Vec2f LRCoord) const
    {
        if (m_numPyramids_==1)
        {
            return LRCoord - getROI_Centre(0) + getTransformedROI_Centre(0);
        } else
            if (m_numPyramids_==2)
            {
                const osg::Vec2f A=getROI_Centre(1) - getROI_Centre(0);
                const float aSq=A.length2();
                const osg::Vec2f B(A._v[1], -A._v[0]);

                const osg::Vec2f tA=getTransformedROI_Centre(1) - getTransformedROI_Centre(0);
                const osg::Vec2f tB(tA._v[1], -tA._v[0]);

                const float xFrac=(LRCoord-getROI_Centre(0)) * A / aSq;
                const float yFrac=(LRCoord-getROI_Centre(0)) * B / aSq;

                return tA*xFrac + tB*yFrac + getTransformedROI_Centre(0);
            } else
                if (m_numPyramids_==4)
                {
                    const float yFrac= (LRCoord._v[1]-getROI_Centre(0).y()) / (getROI_Centre(3).y()-getROI_Centre(0).y());

                    const osg::Vec2f left=( getTransformedROI_Centre(3) - getTransformedROI_Centre(0)) * yFrac + getTransformedROI_Centre(0);
                    const osg::Vec2f right=( getTransformedROI_Centre(2) - getTransformedROI_Centre(1)) * yFrac + getTransformedROI_Centre(1);

                    return (right-left) * ( (LRCoord._v[0]-getROI_Centre(0).x())/(getROI_Centre(1).x()-getROI_Centre(0).x()) ) + left;
                } else
                {
                    logMessage(LOG_CRITICAL) << "WARNING: ImageStabiliserMultiLK does not support a " << m_numPyramids_ << "point homography!!!\n";
                    return osg::Vec2f(0.0f, 0.0f);
                }
    }

    void offsetQuadPositionByROICentres(uint32_t i_ulWidth, uint32_t i_ulHeigt);

public:

    void updateOutputQuadTransform()
    {
        osg::Matrixd deltaTransformMatrix=getDeltaTransformationMatrix();

        //=== Older code with transform filter ===
        for (int filterNum=0; filterNum<(int)(filterPairs_.size()); filterNum++)
        {
            filterPairs_[filterNum].first=filterPairs_[filterNum].first*(1.0f-filterPairs_[filterNum].second)+osg::Vec2d(deltaTransformMatrix(3,0), deltaTransformMatrix(3,2))*(filterPairs_[filterNum].second);
        }

        {//Update output quad's transform with the delta transform.
            osg::Matrixd quadDeltaTransform=deltaTransformMatrix;
            quadDeltaTransform(3,0)=quadDeltaTransform(3,0)-filterPairs_[0].first._v[0];
            quadDeltaTransform(3,2)=quadDeltaTransform(3,2)-filterPairs_[0].first._v[1];

            osg::Matrixd quadTransform=m_rpQuadMatrixTransform->getMatrix();
            quadTransform=quadDeltaTransform*quadTransform;
            m_rpQuadMatrixTransform->setMatrix(quadTransform);
        }
        //===


        //=== Update transformed ROI centres. ===

        if (m_numPyramids_==1)
        {
            m_TransformedROICentreVec[0].first-= m_h[0]._v[0];
            m_TransformedROICentreVec[0].second-= m_h[0]._v[1];
        } else
            if (m_numPyramids_==2)
            {
                float ax, ay, ar, bx, by, br;

                ax=m_TransformedROICentreVec[1].first - m_TransformedROICentreVec[0].first;
                ay=m_TransformedROICentreVec[1].second - m_TransformedROICentreVec[0].second;
                bx=-ay;
                by=ax;

                ar=sqrt(ax*ax+ay*ay);
                ax/=ar;
                ay/=ar;

                float scale=1.0;//(getTransformedROI_Centre(1) - getTransformedROI_Centre(0)).length() / (getROI_Centre(1) - getROI_Centre(0)).length();

                br=sqrt(bx*bx+by*by);
                bx/=br;
                by/=br;

                //=== ROI 0 ===//
                m_TransformedROICentreVec[0].first-=m_h[0]._v[0] * ax * scale + m_h[0]._v[1] * bx * scale;
                m_TransformedROICentreVec[0].second-=m_h[0]._v[0] * ay * scale + m_h[0]._v[1] * by * scale;
                //=== ===//

                //=== ROI 1 ===//
                m_TransformedROICentreVec[1].first-=m_h[1]._v[0] * ax * scale + m_h[1]._v[1] * bx * scale;
                m_TransformedROICentreVec[1].second-=m_h[1]._v[0] * ay * scale + m_h[1]._v[1] * by * scale;
                //=== ===//
            } else
                if (m_numPyramids_==4)
                {
                    float ax, ay, ar, bx, by, br;

                    float scaleTop=(getTransformedROI_Centre(1) - getTransformedROI_Centre(0)).length() / (getROI_Centre(1) - getROI_Centre(0)).length();
                    float scaleBottom=(getTransformedROI_Centre(2) - getTransformedROI_Centre(3)).length() / (getROI_Centre(2) - getROI_Centre(3)).length();
                    float scaleLeft=(getTransformedROI_Centre(3) - getTransformedROI_Centre(0)).length() / (getROI_Centre(3) - getROI_Centre(0)).length();
                    float scaleRight=(getTransformedROI_Centre(2) - getTransformedROI_Centre(1)).length() / (getROI_Centre(2) - getROI_Centre(1)).length();

                    //=== ROI 0 ===//
                    ax=m_TransformedROICentreVec[1].first - m_TransformedROICentreVec[0].first;
                    ay=m_TransformedROICentreVec[1].second - m_TransformedROICentreVec[0].second;
                    ar=sqrt(ax*ax+ay*ay);
                    ax/=ar;
                    ay/=ar;

                    bx=m_TransformedROICentreVec[3].first - m_TransformedROICentreVec[0].first;
                    by=m_TransformedROICentreVec[3].second - m_TransformedROICentreVec[0].second;
                    br=sqrt(bx*bx+by*by);
                    bx/=br;
                    by/=br;

                    m_TransformedROICentreVec[0].first-=m_h[0]._v[0] * ax * scaleTop + m_h[0]._v[1] * bx * scaleLeft;
                    m_TransformedROICentreVec[0].second-=m_h[0]._v[0] * ay * ax * scaleTop + m_h[0]._v[1] * by * scaleLeft;
                    //=== ===//

                    //=== ROI 1 ===//
                    ax=m_TransformedROICentreVec[1].first - m_TransformedROICentreVec[0].first;
                    ay=m_TransformedROICentreVec[1].second - m_TransformedROICentreVec[0].second;
                    ar=sqrt(ax*ax+ay*ay);
                    ax/=ar;
                    ay/=ar;

                    bx=m_TransformedROICentreVec[2].first - m_TransformedROICentreVec[1].first;
                    by=m_TransformedROICentreVec[2].second - m_TransformedROICentreVec[1].second;
                    br=sqrt(bx*bx+by*by);
                    bx/=br;
                    by/=br;

                    m_TransformedROICentreVec[1].first-=m_h[1]._v[0] * ax * scaleTop + m_h[1]._v[1] * bx * scaleRight;
                    m_TransformedROICentreVec[1].second-=m_h[1]._v[0] * ay * scaleTop + m_h[1]._v[1] * by * scaleRight;
                    //=== ===//

                    //=== ROI 2 ===//
                    ax=m_TransformedROICentreVec[2].first - m_TransformedROICentreVec[3].first;
                    ay=m_TransformedROICentreVec[2].second - m_TransformedROICentreVec[3].second;
                    ar=sqrt(ax*ax+ay*ay);
                    ax/=ar;
                    ay/=ar;

                    bx=m_TransformedROICentreVec[2].first - m_TransformedROICentreVec[1].first;
                    by=m_TransformedROICentreVec[2].second - m_TransformedROICentreVec[1].second;
                    br=sqrt(bx*bx+by*by);
                    bx/=br;
                    by/=br;

                    m_TransformedROICentreVec[2].first-=m_h[2]._v[0] * ax * scaleBottom + m_h[2]._v[1] * bx * scaleRight;
                    m_TransformedROICentreVec[2].second-=m_h[2]._v[0] * ay * scaleBottom + m_h[2]._v[1] * by * scaleRight;
                    //=== ===//

                    //=== ROI 2 ===//
                    ax=m_TransformedROICentreVec[2].first - m_TransformedROICentreVec[3].first;
                    ay=m_TransformedROICentreVec[2].second - m_TransformedROICentreVec[3].second;
                    ar=sqrt(ax*ax+ay*ay);
                    ax/=ar;
                    ay/=ar;

                    bx=m_TransformedROICentreVec[3].first - m_TransformedROICentreVec[0].first;
                    by=m_TransformedROICentreVec[3].second - m_TransformedROICentreVec[0].second;
                    br=sqrt(bx*bx+by*by);
                    bx/=br;
                    by/=br;

                    m_TransformedROICentreVec[3].first-=m_h[3]._v[0] * ax * scaleBottom + m_h[3]._v[1] * bx * scaleLeft;
                    m_TransformedROICentreVec[3].second-=m_h[3]._v[0] * ay * scaleBottom + m_h[3]._v[1] * by * scaleLeft;
                    //=== ===//
                } else
                {
                    logMessage(LOG_CRITICAL) << "WARNING: ImageStabiliserMultiLK does not support a " << m_numPyramids_ << "point homography!!!\n";
                }
        //===

        offsetQuadPositionByROICentres(m_pInputTexture->getTextureWidth(), m_pInputTexture->getTextureHeight());
    }

    osg::ref_ptr<osg::Geode> createScreenAlignedQuad(unsigned long i_ulWidth, unsigned long i_ulHeight);
    osg::Camera *createScreenAlignedCamera(unsigned long i_ulWidth, unsigned long i_ulHeight);

    osg::Node* createOutputPass();


    static unsigned long m_ulLKBorder;

    std::vector<std::pair<osg::Vec2f, float> > filterPairs_;//Filtered speed AND filter history factor.
};
}
#endif
