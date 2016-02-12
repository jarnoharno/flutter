#include "options.h"
#include "opt_parser.h"
#include "config.h"
#include <opencv2/opencv.hpp>
#include <iostream>

#ifdef CAN_REDIRECT_TO_DEV_NULL
#include <unistd.h>
#include <fcntl.h>
#endif

using namespace std;

struct file_to_null
{
#ifdef CAN_REDIRECT_TO_DEV_NULL
	file_to_null(FILE* file):
		file(file)
	{
		fflush(file);
		old_err = dup(fileno(file));
		nil_err = open("/dev/null", O_WRONLY);
		dup2(nil_err, fileno(file));
		close(nil_err);
	}

	~file_to_null()
	{
		fflush(file);
		dup2(old_err, fileno(file));
		close(old_err);
	}

	FILE* file;
	int old_err;
	int nil_err;
#else
	file_to_null(FILE*) {}
#endif
};

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
struct fail_exception {};

parse_status parse(options& opts, int argc, char* argv[])
{
	opt::parser op;
	op.add('r', "ransac", &opts.ransac);
	op.add('k', "kalman", &opts.kalman);
	op.add('d', "device", [&](int device) {
		{
			file_to_null ftn(stderr);
			opts.cap = make_unique<cv::VideoCapture>(device);
		}
		if (!opts.cap->isOpened()) {
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
		{
			file_to_null ftn(stdout);
			opts.cap = make_unique<cv::VideoCapture>(file);
		}
		if (!opts.cap->isOpened()) {
			cerr << "unable to open file " << file << endl;
			return fail;
		}
	}
	return cont;
}

