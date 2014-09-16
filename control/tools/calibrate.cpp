/**
 * calibrate images using darks and flats
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <includes.h>
#include <AstroDebug.h>
#include <AstroFormat.h>
#include <iostream>
#include <AstroImage.h>
#include <AstroCalibration.h>
#include <AstroInterpolation.h>
#include <AstroIO.h>
#include <AstroDemosaic.h>
#include <AstroImager.h>
#include <stacktrace.h>

using namespace astro;
using namespace astro::io;
using namespace astro::calibration;
using namespace astro::interpolation;
using namespace astro::camera;

namespace astro {

/**
 * \brief usage
 */
void	usage(const char *progname) {
	std::cout << "usage: " << progname << " [ options ] infile outfile"
		<< std::endl;
	std::cout << "options:" << std::endl;
	std::cout << "  -D dark   use image file <dark> for dark correction"
		<< std::endl;
	std::cout << "  -F flat   use image file <flat> for flat correction"
		<< std::endl;
	std::cout << "  -m min    clamp the image values to at least <min>"
		<< std::endl;
	std::cout << "  -M max    clamp the image values to at most <max>"
		<< std::endl;
	std::cout << "  -b        demosaic bayer images" << std::endl;
	std::cout << "  -i        interpolate bad pixels" << std::endl;
	std::cout << "  -d        increase debug level" << std::endl;
	std::cout << "  -h, -?    show this help message" << std::endl;
}

/**
 * \brief Main function in astro namespace
 */
int	calibrate_main(int argc, char *argv[]) {
	int	c;
	const char	*darkfilename = NULL;
	const char	*flatfilename = NULL;
	double	minvalue = -1;
	double	maxvalue = -1;
	bool	demosaic = false;
	bool	interpolate = false;

	// parse the command line
	while (EOF != (c = getopt(argc, argv, "dD:F:?hm:M:bi")))
		switch (c) {
		case 'd':
			debuglevel = LOG_DEBUG;
			break;
		case 'D':
			darkfilename = optarg;
			break;
		case 'F':
			flatfilename = optarg;
			break;
		case 'm':
			minvalue = atof(optarg);
			break;
		case 'M':
			maxvalue = atof(optarg);
			break;
		case 'b':
			demosaic = true;
			break;
		case 'i':
			interpolate = true;
			break;
		case '?':
		case 'h':
			usage(argv[0]);
			return EXIT_SUCCESS;
			break;
		}

	// two more arguments are required: infile and outfile
	if (2 != argc - optind) {
		std::string	msg("wrong number of arguments");
		debug(LOG_ERR, DEBUG_LOG, 0, "%s", msg.c_str());
		throw std::runtime_error(msg);
	}
	std::string	infilename(argv[optind++]);
	std::string	outfilename(argv[optind]);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "calibrate %s to %s",
		infilename.c_str(), outfilename.c_str());

	// read the infile
	FITSin	infile(infilename);
	ImagePtr	image = infile.read();

	// build the Imager
	Imager	imager;

	// if we have a dark correction, apply it
	ImagePtr	dark;
	if (NULL != darkfilename) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "dark correct: %s",
			darkfilename);
		FITSin	darkin(darkfilename);
		dark = darkin.read();
		imager.dark(dark);
		imager.darksubtract(true);
	}

	// if we have a flat file, we perform flat correction
	if (NULL != flatfilename) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "flat correction: %s",
			flatfilename);
		FITSin	flatin(flatfilename);
		ImagePtr	flat = flatin.read();
		imager.flat(flat);
		imager.flatdivide(true);
	}

	// perform bad pixel interpolation
	if (interpolate) {
		imager.interpolate(true);
	}

	// apply imager corrections
	imager(image);

	// if minvalue or maxvalue are set, clamp the image values
	if ((minvalue >= 0) || (maxvalue >= 0)) {
		if (minvalue < 0) {
			minvalue = 0;
		}
		if (maxvalue < 0) {
			maxvalue = std::numeric_limits<double>::infinity();
		}
		Clamper	clamp(minvalue, maxvalue);
		clamp(image);
	}

	// after all the calibrations have been performed, write the output
	// file
	FITSout	outfile(outfilename);

	// if demosaic is requested we do that now
	if (demosaic) {
		ImagePtr	demosaiced = demosaic_bilinear(image);
		outfile.write(demosaiced);
	} else {
		outfile.write(image);
	}

	// that's it
	return EXIT_SUCCESS;
}

} // namespace astro

int	main(int argc, char *argv[]) {
	signal(SIGSEGV, stderr_stacktrace);
	try {
		return astro::calibrate_main(argc, argv);
	} catch (const std::exception& x) {
		std::cerr << "terminated by ";
		std::cerr << astro::demangle(typeid(x).name()) << ": ";
		std::cerr << x.what() << std::endl;
	} catch (...) {
		std::cerr << "terminated by unknown exception" << std::endl;
	}
	return EXIT_FAILURE;
}
