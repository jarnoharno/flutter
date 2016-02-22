#ifndef OPTIONS_IO_H
#define OPTIONS_IO_H

#include "options.h"
#include <iostream>

namespace flutter
{

inline char const* bool_str(bool b)
{
	return b ? "true" : "false";
}

inline char const* input_str(input_source src)
{
	switch (src) {
	case device_input:
		return "device_input";
	case file_input:
		return "file_input";
	}
	return "";
}

inline std::ostream& operator<<(std::ostream& o, options const& opts)
{
	using namespace std;
	return o <<
		"{" << endl <<
		"  ransac: " << opts.ransac << "," << endl <<
		"  process_error: " << opts.process_error << "," << endl <<
		"  measurement_error: " << opts.measurement_error << "," << endl <<
		"  low_pass: " << opts.low_pass << "," << endl <<
		"  fps: " << opts.fps << "," << endl <<
		"  delay: " << opts.delay << "," << endl <<
		"  quiet: " << bool_str(opts.quiet) << "," << endl <<
		"  codec: " << opts.codec << "," << endl <<
		"  fourcc: 0x" << hex << opts.fourcc << dec << "," << endl <<
		"  input_src: " << input_str(opts.input_src) << "," << endl <<
		"  input_file: \"" << opts.input_file << "\"," << endl <<
		"  output_file: \"" << opts.output_file << "\"," << endl <<
		"  trajectory_file: \"" << opts.trajectory_file << "\"," << endl <<
		"  out_width: " << opts.out_width << "," << endl <<
		"  out_height: " << opts.out_height << "," << endl <<
		"}";
}

}

#endif // OPTIONS_IO_H
