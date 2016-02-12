#ifndef SUPPRESS_H
#define SUPPRESS_H

#include "config.h"
#include <cstdio>
#ifdef CAN_REDIRECT_TO_DEV_NULL
#include <unistd.h>
#include <fcntl.h>
#endif

struct file_to_null
{
#ifdef CAN_REDIRECT_TO_DEV_NULL
	inline file_to_null(FILE* file):
		file(file)
	{
		fflush(file);
		old_err = dup(fileno(file));
		nil_err = open("/dev/null", O_WRONLY);
		dup2(nil_err, fileno(file));
		close(nil_err);
	}

	inline ~file_to_null()
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

struct std_to_null
{
	inline std_to_null():
		out(stdout),
		err(stderr)
	{
	}

	file_to_null out;
	file_to_null err;
};

#endif // SUPPRESS_H
