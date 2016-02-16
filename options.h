#ifndef OPTIONS_H
#define OPTIONS_H

#include <memory>
#include <string>

namespace cv {
	class VideoCapture;
	class VideoWriter;
}

namespace flutter {

struct options {
	float ransac;
	float kalman;
	float low_pass;
	float fps;
	int delay;
	bool quiet;
	std::string codec;
	int fourcc;
	std::string input;
	std::string output;
	int out_width;
	int out_height;
	std::unique_ptr<cv::VideoCapture> capture;
	std::unique_ptr<cv::VideoWriter> writer;

	options();
};

enum parse_status {
	fail,
	stop,
	cont
};

parse_status parse(options& opts, int argc, char* argv[]);

}

#endif // OPTIONS_H
