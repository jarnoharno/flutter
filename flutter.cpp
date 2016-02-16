#include "options.h"
#include "options_io.h"
#include <opencv2/opencv.hpp>
#include <iostream>

using namespace std;
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
	return EXIT_SUCCESS;
}
