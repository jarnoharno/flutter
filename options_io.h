#ifndef OPTIONS_IO_H
#define OPTIONS_IO_H

#include "options.h"
#include <iostream>

namespace flutter
{

inline char const* bool_string(bool b)
{
	return b ? "true" : "false";
}

inline std::ostream& operator<<(std::ostream& o, options const& opts)
{
	using namespace std;
	return o <<
		"{" << endl <<
		"  ransac: " << opts.ransac << "," << endl <<
		"  kalman: " << opts.kalman << "," << endl <<
		"  low_pass: " << opts.low_pass << "," << endl <<
		"  fps: " << opts.fps << "," << endl <<
		"  delay: " << opts.delay << "," << endl <<
		"  quiet: " << bool_string(opts.quiet) << "," << endl <<
		"}";
}

}

#endif // OPTIONS_IO_H
