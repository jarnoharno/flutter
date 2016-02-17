#include "options.h"
#include "options_io.h"
#include "transform.h"
#include <opencv2/opencv.hpp>
#include <iostream>

using namespace std;
using namespace cv;

namespace flutter {

static char const* const program_name = "flutter";

struct state {
	options opts;

	Mat prev_frame;
	Mat next_frame;

	void init();
	void run();
	bool display();
};

void state::init()
{
	*opts.capture >> next_frame;
	next_frame.copyTo(prev_frame);
}

void state::run()
{
	namedWindow(program_name, CV_WINDOW_NORMAL);
	bool running = display();
	while (running) {
		next_frame.copyTo(prev_frame);
		*opts.capture >> next_frame;
		running = display();
	}
}

bool state::display()
{
	if (!opts.quiet) {
		Size size(opts.out_width, opts.out_height);
		Mat display;
		resize(next_frame, display, size);
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
	st.init();
	st.run();
	return EXIT_SUCCESS;
}
