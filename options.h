#ifndef OPTIONS_H
#define OPTIONS_H

#include <memory>
#include <string>

namespace cv
{
class VideoCapture;
class VideoWriter;
}

namespace flutter
{

enum input_source {
        device_input,
        file_input
};

struct options {
	float ransac;
	float kalman;
	float low_pass;
	float fps;
	int delay;
	bool quiet;
	std::string codec;
	int fourcc;
	input_source input_src;
	std::string input_file;
	std::string output_file;
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
