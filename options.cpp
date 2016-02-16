#include "options.h"
#include "opt_parser.h"
#include "suppress.h"
#include <opencv2/opencv.hpp>
#include <iostream>

using namespace std;

static const flutter::options default_opts;

static void print_help()
{
	cout <<
		"flutter - real time video stabilizer\n"
		"usage: flutter [options] [infile [outfile]]\n"
		"\n"
		"  -h, --help             Display this help and exit\n"
		"  -r, --ransac=<float>   Maximum inlier distance relative to image dimensions.\n"
		"                         The default is " << default_opts.ransac <<  ".\n"
		"  -k, --kalman=<float>   Kalman measurement error relative to image dimensions.\n"
		"                         The default is " << default_opts.kalman << ".\n"
		"  -l, --low-pass=<float> Low pass filter magnitude. The default is " << default_opts.low_pass << ".\n"
		"  -d, --device=<int>     Input device number. The default is 0.\n"
		"                         If infile is given, it overrides this setting.\n"
		"  -f, --fps=<float>      Frames per second. Only relevant when output is shown.\n"
		"                         The default is " << default_opts.fps << ".\n"
		"  -q, --quiet            Do not show output.\n"
		;
}

struct stop_exception {};
struct fail_exception {};

template <typename T = int>
static bool get_video_capture(unique_ptr<cv::VideoCapture>& vc, const T& src = 0)
{
	std_to_null stn;
	unique_ptr<cv::VideoCapture> tmp = make_unique<cv::VideoCapture>(src);
	if (!tmp->isOpened())
		return false;
	vc.swap(tmp);
	return true;
}

flutter::parse_status flutter::parse(options& opts, int argc, char* argv[])
{
	opt::parser op;
	op.add('r', "ransac", &opts.ransac);
	op.add('k', "kalman", &opts.kalman);
	op.add('f', "fps", &opts.fps);
	op.add('q', "quiet", &opts.quiet);
	op.add('d', "device", [&](int device) {
		if (!get_video_capture(opts.cap, device)) {
			cerr << "unable to open device " << device << endl;
			throw fail_exception();
		}
	});
	op.add('h', "help", []() {
		print_help();
		throw stop_exception();
	});
	try {
		op.parse(argc, argv);
	} catch (const opt::parse_error& err) {
		cerr << "parse error: " << err.what() << endl;
		return fail;
	} catch (const opt::no_argument_error& err) {
		cerr << "no argument expected: " << err.what() << endl;
		return fail;
	} catch (const opt::required_argument_error& err) {
		cerr << "argument expected: " << err.what() << endl;
		return fail;
	} catch (const opt::unknown_option_error& err) {
		cerr << "unknown option: " << err.what() << endl;
		return fail;
	} catch (const stop_exception& err) {
		return stop;
	} catch (const fail_exception& err) {
		return fail;
	}
	if (op.pos_args.size() > 1) {
		cerr << "at most one infile expected" << endl;
		return fail;
	} else if (op.pos_args.size() == 1) {
		const string& file = op.pos_args.front();
		if (!get_video_capture(opts.cap, file)) {
			cerr << "unable to open file " << file << endl;
			return fail;
		}
	}
	if (!opts.cap && !get_video_capture(opts.cap)) {
		cerr << "unable to open default device" << endl;
		return fail;
	}
	opts.delay = 1000/opts.fps;
	return cont;
}

