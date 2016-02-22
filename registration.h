#ifndef REGISTRATION_H
#define REGISTRATION_H

#include <opencv2/opencv.hpp>

namespace flutter {

cv::Mat estimate_rigid_transform(cv::InputArray src1, cv::InputArray src2);

}

#endif // REGISTRATION_H
