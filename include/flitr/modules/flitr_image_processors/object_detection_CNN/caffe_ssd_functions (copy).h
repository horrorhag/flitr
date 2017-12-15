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

double getElapsedTime(timeval startTime, timeval endTime){
    long seconds  = endTime.tv_sec  - startTime.tv_sec;
    long useconds = endTime.tv_usec - startTime.tv_usec;    //decimal sec

    long milliSec = ((seconds) * 1000 + useconds/1000.0) + 0.5;
    return (double) milliSec/1000;
}

class SingleShotDetector {
 public:
  SingleShotDetector(const string& model_file, const string& weights_file, const string& mean_file,
           const string& mean_value,
           const string& labels_filename
  );

  std::vector<vector<float> > Detect(const cv::Mat& img);

 void forwardImage(Mat &image);

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

SingleShotDetector::SingleShotDetector(const string& model_file,const string& weights_file,
                   const string& mean_file,
                   const string& mean_value,
                   const string& labels_filename) {

#ifdef CPU_ONLY
  Caffe::set_mode(Caffe::CPU);
#else
  Caffe::set_mode(Caffe::GPU);
#endif


  /* Load the network. */
  net_.reset(new Net<float>(model_file, TEST));
  net_->CopyTrainedLayersFrom(weights_file);

  CHECK_EQ(net_->num_inputs(), 1) << "Network should have exactly one input.";
  CHECK_EQ(net_->num_outputs(), 1) << "Network should have exactly one output.";

  Blob<float>* input_layer = net_->input_blobs()[0];
  num_channels_ = input_layer->channels();
  CHECK(num_channels_ == 3 || num_channels_ == 1)
    << "Input layer should have 1 or 3 channels.";
  input_geometry_ = cv::Size(input_layer->width(), input_layer->height());

  /* Load the binaryproto mean file. */
#ifndef GFLAGS_GFLAGS_H_
  namespace gflags = google;
#endif

  //Set the Mean file or value
  SetMean(mean_file, mean_value);

  //![ Load class names for the index labels]
  //Read Labels textfile
  string textLine;
  std::ifstream labels_file(labels_filename);

  if(labels_file.is_open()){

      while(std::getline(labels_file,textLine)){
          ClassNames.push_back(textLine);
      }
  }
  labels_file.close();
  cout<<"Considered ..." << ClassNames.size() <<" labels from " <<labels_filename <<endl;

}

std::vector<vector<float> > SingleShotDetector::Detect(const cv::Mat& img) {
struct timeval start, end;
gettimeofday(&start, NULL);



  Blob<float>* input_layer = net_->input_blobs()[0];
  input_layer->Reshape(1, num_channels_,
                       input_geometry_.height, input_geometry_.width);
  /* Forward dimension change to all layers. */
  net_->Reshape();

  std::vector<cv::Mat> input_channels;
  WrapInputLayer(&input_channels);

  Preprocess(img, &input_channels);

gettimeofday(&end, NULL);
cout<<"Time(s) Elapsed (A):" << getElapsedTime(start,end) <<endl;



  net_->Forward();

  /* Copy the output layer to a std::vector */
  Blob<float>* result_blob = net_->output_blobs()[0];
  const float* result = result_blob->cpu_data();
  const int num_det = result_blob->height();
  vector<vector<float> > detections;
  for (int k = 0; k < num_det; ++k) {
    if (result[0] == -1) {
      // Skip invalid detection.
      result += 7;
      continue;
    }
    vector<float> detection(result, result + 7);
    detections.push_back(detection);
    result += 7;
  }
  return detections;
}

/* Load the mean file in binaryproto format. */
void SingleShotDetector::SetMean(const string& mean_file, const string& mean_value) {
  cv::Scalar channel_mean;

  if (!mean_file.empty()) {
    if(!mean_value.empty())
        std::cerr<<"Cannot specify mean_file and mean_value at the same time" <<endl;

    BlobProto blob_proto;
    ReadProtoFromBinaryFileOrDie(mean_file.c_str(), &blob_proto);

    /* Convert from BlobProto to Blob<float> */
    Blob<float> mean_blob;
    mean_blob.FromProto(blob_proto);
    CHECK_EQ(mean_blob.channels(), num_channels_)
      << "Number of channels of mean file doesn't match input layer.";

    /* The format of the mean file is planar 32-bit float BGR or grayscale. */
    std::vector<cv::Mat> channels;
    float* data = mean_blob.mutable_cpu_data();
    for (int i = 0; i < num_channels_; ++i) {
      /* Extract an individual channel. */
      cv::Mat channel(mean_blob.height(), mean_blob.width(), CV_32FC1, data);
      channels.push_back(channel);
      data += mean_blob.height() * mean_blob.width();
    }

    /* Merge the separate channels into a single image. */
    cv::Mat mean;
    cv::merge(channels, mean);

    /* Compute the global mean pixel value and create a mean image
     * filled with this value. */
    channel_mean = cv::mean(mean);
    mean_ = cv::Mat(input_geometry_, mean.type(), channel_mean);

  }
  if (!mean_value.empty()) {

    if(!mean_file.empty())
        std::cerr<<  "Cannot specify mean_file and mean_value at the same time" <<endl;

    stringstream ss(mean_value);
    vector<float> values;
    string item;
    while (getline(ss, item, ',')) {
      float value = std::atof(item.c_str());
      values.push_back(value);
    }
    CHECK(values.size() == (uint) 1 || values.size() == (uint) num_channels_) <<
      "Specify either 1 mean_value or as many as channels: " << num_channels_;

    std::vector<cv::Mat> channels;
    for (int i = 0; i < num_channels_; ++i) {
      /* Extract an individual channel. */
      cv::Mat channel(input_geometry_.height, input_geometry_.width, CV_32FC1,
          cv::Scalar(values[i]));
      channels.push_back(channel);
    }
    cv::merge(channels, mean_);
  }
}

/* Wrap the input layer of the network in separate cv::Mat objects
 * (one per channel). This way we save one memcpy operation and we
 * don't need to rely on cudaMemcpy2D. The last preprocessing
 * operation will write the separate channels directly to the input
 * layer. */
void SingleShotDetector::WrapInputLayer(std::vector<cv::Mat>* input_channels) {
  Blob<float>* input_layer = net_->input_blobs()[0];

  int width = input_layer->width();
  int height = input_layer->height();
  float* input_data = input_layer->mutable_cpu_data();
  for (int i = 0; i < input_layer->channels(); ++i) {
    cv::Mat channel(height, width, CV_32FC1, input_data);
    input_channels->push_back(channel);
    input_data += width * height;
  }
}

void SingleShotDetector::Preprocess(const cv::Mat& img, std::vector<cv::Mat>* input_channels){
  /* Convert the input image to the input image format of the network. */
  cv::Mat sample;
  if (img.channels() == 3 && num_channels_ == 1)
    cv::cvtColor(img, sample, cv::COLOR_BGR2GRAY);
  else if (img.channels() == 4 && num_channels_ == 1)
    cv::cvtColor(img, sample, cv::COLOR_BGRA2GRAY);
  else if (img.channels() == 4 && num_channels_ == 3)
    cv::cvtColor(img, sample, cv::COLOR_BGRA2BGR);
  else if (img.channels() == 1 && num_channels_ == 3)
    cv::cvtColor(img, sample, cv::COLOR_GRAY2BGR);
  else
    sample = img;

  cv::Mat sample_resized;
  if (sample.size() != input_geometry_)
    cv::resize(sample, sample_resized, input_geometry_);
  else
    sample_resized = sample;

  cv::Mat sample_float;
  if (num_channels_ == 3)
    sample_resized.convertTo(sample_float, CV_32FC3);
  else
    sample_resized.convertTo(sample_float, CV_32FC1);

  cv::Mat sample_normalized;
  cv::subtract(sample_float, mean_, sample_normalized);

  /* This operation will write the separate BGR planes directly to the
   * input layer of the network because it is wrapped by the cv::Mat
   * objects in input_channels. */
  cv::split(sample_normalized, *input_channels);

  CHECK(reinterpret_cast<float*>(input_channels->at(0).data)
        == net_->input_blobs()[0]->cpu_data())
    << "Input channels are not wrapping the input layer of the network.";
}


void SingleShotDetector::forwardImage(cv::Mat &image){

  // Process image one by one.
    if(image.empty())
        std::cout<<"Unable to open the opencv image";


    std::vector<vector<float>> detections = this->Detect(image);  //SingleShotDetector.Detect(image);

    const float confidence_threshold = 0.5;

    /* Print the detection results. */
    for (uint i = 0; i < detections.size(); ++i) {
        const vector<float>& d = detections[i];

        // Detection format: [image_id, label, score, xmin, ymin, xmax, ymax].

        const int classId = d[1];
        const float score = d[2];


        if (score >= confidence_threshold) {

            float xLeftBottom = d[3] * image.cols;
            float yLeftBottom = d[4] * image.rows;
            float xRightTop =   d[5] * image.cols;
            float yRightTop =   d[5] * image.rows;

            std::cout << "Class: " << classId << " = " << ClassNames.at(classId) <<std::endl;

            std::cout << "Confidence: " << d[2] << std::endl;


            cv::Rect myRectangle(xLeftBottom, yLeftBottom,xRightTop - xLeftBottom,yRightTop - yLeftBottom);

            cv::rectangle(image, myRectangle, Scalar(0, 255, 0));

            /*
            out << out_file << "_";
            out << std::setfill('0') << std::setw(6) << frame_count << " ";
            out << static_cast<int>(d[1]) << " ";
            out << score << " ";
            out << static_cast<int>(d[3] * img.cols) << " ";
            out << static_cast<int>(d[4] * img.rows) << " ";
            out << static_cast<int>(d[5] * img.cols) << " ";
            out << static_cast<int>(d[6] * img.rows) << std::endl;
            */


            cv::waitKey(10);
            if(i==0){
                //Display top class on the image
                cv::putText(image, cv::String(ClassNames.at(classId)) , cvPoint(30,30),
                    FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(100,255,100), 1, CV_AA);

            }

        }

    }





}
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
