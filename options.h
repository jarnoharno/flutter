#ifndef OPTIONS_H
#define OPTIONS_H

#include <memory>

namespace cv {
	class VideoCapture;
}

struct options {
	float ransac = 0.001;
	float kalman = 0.5;
	float low_pass = 0.1;
	std::unique_ptr<cv::VideoCapture> cap;
};

enum parse_status {
	fail,
	stop,
	cont
};

parse_status parse(options& opts, int argc, char* argv[]);

#endif // OPTIONS_H
