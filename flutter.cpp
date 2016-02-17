#include "options.h"
#include "options_io.h"
#include "transform.h"
#include <opencv2/opencv.hpp>
#include <iostream>

using namespace std;
using namespace cv;

namespace flutter {

static char const* const program_name = "flutter";

struct frame {
	Mat image;
	Transform<double> raw_transform;
	Transform<double> camera_transform;
	Transform<double> apparent_transform;
};

struct state {
	options opts;

	frame prev_frame;
	frame next_frame;

	void init();
	void run();
	bool display();
};

void state::run()
{
	namedWindow(program_name, CV_WINDOW_NORMAL);
	if (!opts.capture->read(next_frame.image))
		return;
	bool running = display();
	while (running) {
		next_frame.image.copyTo(prev_frame.image);
		if (!opts.capture->read(next_frame.image))
			return;
		running = display();
	}
}

bool state::display()
{
	Size size(opts.out_width, opts.out_height);
	Mat display;
	resize(next_frame.image, display, size);
	if (opts.writer) {
		opts.writer->write(display);
	}
	if (!opts.quiet) {
		imshow(program_name, display);
		waitKey(opts.delay);
	}
	if (!opts.quiet || opts.input_src == device_input) {
		int key = waitKey(opts.delay);
		switch (key) {
		case 27:
		case 'q':
			return false;
		case 'r':
			resizeWindow(program_name, opts.out_width,
				opts.out_height);
			break;
		}
	}
	return true;
}

}

int main(int argc, char* argv[])
{
	using namespace flutter;
	state st;
	switch (parse(st.opts, argc, argv)) {
	case fail:
		return EXIT_FAILURE;
	case stop:
		return EXIT_SUCCESS;
	case cont:
		break;
	}
	cout << "we're ready!" << endl;
	cout << st.opts << endl;
	st.run();
	return EXIT_SUCCESS;
}
