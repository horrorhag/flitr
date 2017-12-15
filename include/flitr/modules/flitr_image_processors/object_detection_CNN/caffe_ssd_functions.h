#ifndef CAFFE_SSD_FUNCTIONS_H
#define CAFFE_SSD_FUNCTIONS_H 1

#ifdef FLITR_USE_CAFFE
    #include <caffe/caffe.hpp>

    #include <algorithm>
    #include <iomanip>
    #include <iosfwd>
    #include <memory>
    #include <string>
    #include <utility>
    #include <vector>

    #include <istream>

#ifdef USE_OPENCV
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace caffe;  // NOLINT(build/namespaces)

using std::cout; using std::endl;


class SingleShotDetector {
public:
    SingleShotDetector(const string& model_file, const string& weights_file, const string& mean_file,
        const string& mean_value,
        const string& labels_filename
    );

    std::vector<vector<float> > Detect(const cv::Mat& img);

    void forwardImage(cv::Mat &image);

private:
    void SetMean(const string& mean_file, const string& mean_value);

    void WrapInputLayer(std::vector<cv::Mat>* input_channels);

    void Preprocess(const cv::Mat& img,
                  std::vector<cv::Mat>* input_channels);

private:
    std::shared_ptr<Net<float> > net_;
    cv::Size input_geometry_;
    int num_channels_;
    cv::Mat mean_;
    int frame_count;


    std::vector<string> ClassNames;
};

#else
int main(int argc, char** argv) {
  cerr << "This example requires OpenCV; compile with USE_OPENCV." <<endl;
}
#endif  // USE_OPENCV

#else
int main(int argc, char** argv) {
  cerr << "This example requires CAFFE; compile with FLITR_USE_CAFFE." <<endl;
}
#endif //FLITR_USE_CAFFE


#endif //IFNDEF Header file
