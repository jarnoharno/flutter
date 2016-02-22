#include "options.h"
#include "options_io.h"
#include "transform.h"
#include "registration.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <memory>

typedef double t_type;

template <typename T>
struct opencv_traits {};

template <>
struct opencv_traits<float>
{
	static constexpr int type = CV_32FC1;
};

template <>
struct opencv_traits<double>
{
	static constexpr int type = CV_64FC1;
};

using namespace std;
using namespace cv;

namespace flutter {

static char const* const program_name = "flutter";

struct frame {
	Mat image;
	Transform<t_type> sensor;
	Transform<t_type> camera;
	Transform<t_type> apparent;

	void copyTo(frame& f) const;
};

void frame::copyTo(frame& f) const
{
	image.copyTo(f.image);
	f.camera = camera;
	f.apparent = apparent;
}

struct state {
	options opts;

	unique_ptr<frame> prev_frame;
	unique_ptr<frame> next_frame;
	KalmanFilter delta_filter;
	int frame_no;

	state(options opts);
	void run();
	void init();
	void init_filter();
	bool capture();
	bool display();
	void write_trajectory_header();

	void compute_transformation();
};

state::state(options opts):
	opts(move(opts)),
	prev_frame(make_unique<frame>()),
	next_frame(make_unique<frame>()),
	delta_filter(3,3,0,opencv_traits<t_type>::type),
	frame_no(0)
{
}

void state::compute_transformation()
{
	Mat sensor_delta_mat = estimate_rigid_transform(
		prev_frame->image, next_frame->image);
	if (sensor_delta_mat.empty())
		sensor_delta_mat = Mat::eye(2, 3, opencv_traits<t_type>::type);
	Transform<t_type> sensor_delta(sensor_delta_mat);
	Mat sensor_delta_vec = sensor_delta.toVec();
	delta_filter.predict();
	Transform<t_type> camera_delta = Transform<t_type>::fromVec(
		delta_filter.correct(sensor_delta_vec));
	next_frame->sensor = prev_frame->sensor + sensor_delta;
	next_frame->camera = prev_frame->camera + camera_delta;
	next_frame->apparent = prev_frame->apparent +
		opts.low_pass * (next_frame->camera - prev_frame->apparent);
}

bool state::capture()
{
	return opts.capture->read(next_frame->image);
}

void state::run()
{
	if (!capture())
		return;
	init();
	if (!display())
		return;
	for (;;) {
		prev_frame.swap(next_frame);
		if (!capture())
			return;
		compute_transformation();
		if (!display())
			return;
	}
}

void state::init()
{
	namedWindow(program_name, CV_WINDOW_NORMAL);
	write_trajectory_header();
	init_filter();
}

static constexpr char delim = '\t';

void state::write_trajectory_header()
{
	if (!opts.trajectory)
		return;
	*opts.trajectory <<
		"frame" << delim <<
		"sensor_x" << delim <<
		"sensor_y" << delim <<
		"sensor_a" << delim <<
		"camera_x" << delim <<
		"camera_y" << delim <<
		"camera_a" << delim <<
		"apparent_x" << delim <<
		"apparent_y" << delim <<
		"apparent_a" << endl;
}

void state::init_filter()
{
	Size size = next_frame->image.size();
	cout << "input size: " << size << endl;
	setIdentity(delta_filter.transitionMatrix);
	setIdentity(delta_filter.measurementMatrix);
	double perr2 = opts.process_error*opts.process_error;
	double merr2 = opts.measurement_error*opts.measurement_error;
	delta_filter.processNoiseCov = (Mat_<t_type>(3,3) <<
		size.width*size.width*perr2,   0, 0, 0,
		size.height*size.height*perr2, 0, 0, 0,
		4*M_PI*M_PI*perr2);
	delta_filter.measurementNoiseCov = (Mat_<t_type>(3,3) <<
		size.width*size.width*perr2,   0, 0, 0,
		size.height*size.height*perr2, 0, 0, 0,
		4*M_PI*M_PI*perr2);
}

bool state::display()
{
	Mat transformed;
	Mat inverse = (next_frame->apparent - next_frame->camera).toMat();
	warpAffine(next_frame->image, transformed, inverse, next_frame->image.size());
	Size size(opts.out_width, opts.out_height);
	Mat display;
	resize(transformed, display, size);
	if (opts.writer) {
		opts.writer->write(display);
	}
	if (opts.trajectory) {
		*opts.trajectory <<
			frame_no << delim <<
			with_delim<delim>(next_frame->sensor) << delim <<
			with_delim<delim>(next_frame->camera) << delim <<
			with_delim<delim>(next_frame->apparent) << endl;
	}
	++frame_no;
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

using namespace flutter;

int main(int argc, char* argv[])
{
	options opts;
	switch (parse(opts, argc, argv)) {
	case fail:
		return EXIT_FAILURE;
	case stop:
		return EXIT_SUCCESS;
	case cont:
		break;
	}
	cout << "we're ready!" << endl;
	cout << opts << endl;
	state st(move(opts));
	st.run();
	return EXIT_SUCCESS;
}
