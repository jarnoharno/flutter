#include "options.h"
#include "opt_parser.h"
#include <opencv2/opencv.hpp>
#include <iostream>

using namespace std;

static void print_help()
{
	cout <<
		"flutter - real time video stabilizer\n"
		"usage: flutter [options] [infile]\n"
		"\n"
		"  -h, --help             Display this help and exit\n"
		"  -r, --ransac=<float>   Maximum inlier distance relative to image dimensions.\n"
		"                         The default is 0.001.\n"
		"  -k, --kalman=<float>   Kalman measurement error relative to image dimensions.\n"
		"                         The default is 0.5.\n"
		"  -l, --low-pass=<float> Low pass filter magnitude. The default is 0.1\n"
		"  -d, --device=<int>     Input device number. The default is 0.\n"
		"                         If infile is given, it overrides this setting.\n"
		;
}

struct stop_exception {};

parse_status parse(options& opts, int argc, char* argv[])
{
	opt::parser op;
	op.add('r', "ransac", &opts.ransac);
	op.add('k', "kalman", &opts.kalman);
	op.add('d', "device", [&](int device) {
		cout << "device=" << device << endl;
		opts.cap = make_unique<cv::VideoCapture>(device);
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
	}
	return cont;
}

