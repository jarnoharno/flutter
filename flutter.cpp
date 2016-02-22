#include "options.h"
#include "options_io.h"
#include "transform.h"
#include "registration.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <memory>
#include <deque>

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
	deque<frame> queue;
	KalmanFilter delta_filter;
	int frame_no;


	state(options opts);
	void run();
	bool init();
	void init_filter();
	bool capture();
	void advance();
	bool display();
	void write_trajectory_header();
	void compute_transformation();
	void compute_apparent();
	void close();
};

state::state(options opts):
	opts(move(opts)),
	delta_filter(3,3,0,opencv_traits<t_type>::type),
	frame_no(0)
{
}

void state::compute_transformation()
{
	frame& prev_frame = queue[1];
	frame& next_frame = queue[0];
	if (prev_frame.image.empty() || next_frame.image.empty())
		return;
	Mat sensor_delta_mat = estimate_rigid_transform(
		prev_frame.image, next_frame.image);
	if (sensor_delta_mat.empty())
		sensor_delta_mat = Mat::eye(2, 3, opencv_traits<t_type>::type);
	Transform<t_type> sensor_delta(sensor_delta_mat);
	Mat sensor_delta_vec = sensor_delta.toVec();
	delta_filter.predict();
	Transform<t_type> camera_delta = Transform<t_type>::fromVec(
		delta_filter.correct(sensor_delta_vec));
	next_frame.sensor = prev_frame.sensor + sensor_delta;
	next_frame.camera = prev_frame.camera + camera_delta;
	compute_apparent();
}

void state::compute_apparent()
{
	frame& prev_frame = queue[1];
	frame& next_frame = queue[0];
	if (opts.avg_window) {
		next_frame.apparent = prev_frame.apparent +
			next_frame.camera / opts.avg_window;
	} else {
		next_frame.apparent = prev_frame.apparent + opts.low_pass *
			(next_frame.camera - prev_frame.apparent);
	}
}

void state::advance()
{
	if (opts.avg_window) {
		queue.front().apparent -= queue.back().camera / opts.avg_window;
	}
	queue.pop_back();
}

bool state::capture()
{
	queue.emplace_front();
	bool ok = opts.capture->read(queue.front().image);
	if (!ok)
		queue.pop_front();
	return ok;
}

void state::close()
{
	if (opts.avg_window && !opts.output_file.empty()) {
		for (int i = 0; i < opts.avg_window/2; ++i) {
			queue.emplace_front();
			queue[0].camera = queue[1].camera;
			compute_apparent();
			advance();
			display();
		}
	}
	cout << "output frames: " << frame_no << endl;
}

void state::run()
{
	if (!capture())
		return;
	if (!init())
		return;
	if (!display())
		return;
	for (;;) {
		if (!capture())
			break;
		compute_transformation();
		advance();
		if (!display())
			break;
	}
	close();
}

bool state::init()
{
	namedWindow(program_name, CV_WINDOW_NORMAL);
	write_trajectory_header();
	init_filter();
	if (!opts.avg_window)
		return true;
	cout << "buffering...";
	for (int i = opts.avg_window/2+1; i < opts.avg_window; ++i)
		queue.emplace_back();
	for (int i = 0; i < opts.avg_window/2; ++i) {
		if (!opts.quiet || opts.input_src == device_input) {
			int key = waitKey(opts.delay);
			switch (key) {
			case 27:
			case 'q':
				return false;
			}
		}
		if (!capture())
			return false;
		compute_transformation();
	}
	cout << " done." << endl;
	return true;
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
	Size size = queue.front().image.size();
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
	const frame& next_frame = queue[0];
	const frame& disp_frame = opts.avg_window ?
		queue[opts.avg_window/2] :
		queue[0];
	Mat inverse = (next_frame.apparent - disp_frame.camera).toMat();
	warpAffine(disp_frame.image, transformed, inverse,
		disp_frame.image.size());
	Size size(opts.out_width, opts.out_height);
	Mat display;
	resize(transformed, display, size);
	if (opts.writer) {
		opts.writer->write(display);
	}
	if (opts.trajectory) {
		*opts.trajectory <<
			frame_no << delim <<
			with_delim<delim>(disp_frame.sensor) << delim <<
			with_delim<delim>(disp_frame.camera) << delim <<
			with_delim<delim>(next_frame.apparent) << '\n';
	}
	++frame_no;
	if (!opts.quiet) {
		imshow(program_name, display);
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
	cout << opts << endl;
	state st(move(opts));
	st.run();
	return EXIT_SUCCESS;
}
