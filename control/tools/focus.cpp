/*
 * focus.cpp -- command line focus utility
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <AstroDebug.h>
#include <stdexcept>
#include <iostream>
#include <AstroFormat.h>
#include <AstroLoader.h>
#include <AstroCamera.h>
#include <AstroDevice.h>
#include <AstroFilter.h>
#include <AstroFilterfunc.h>
#include <AstroIO.h>
#include <AstroUtils.h>
#include <includes.h>

using namespace astro;
using namespace astro::module;
using namespace astro::camera;
using namespace astro::device;
using namespace astro::image;
using namespace astro::image::filter;
using namespace astro::io;

namespace astro {
namespace app {
namespace focus {

static void	usage(const char *progname) {
	Path	p(progname);
	std::cout << "usage:" << std::endl;
	std::cout << std::endl;
	std::cout << "    " << p.basename() << " [ options ]" << std::endl;
	std::cout << std::endl;
	std::cout << "Take an image and compute a focus figure of merit"
		<< std::endl;
	std::cout << std::endl;
	std::cout << "options:" << std::endl;
	std::cout << std::endl;
	std::cout << "    -C,--camera=<cameraid>   select camera id"
		<< std::endl;
	std::cout << "    -c,--ccd=<ccdid>         select ccd id"
		<< std::endl;
	std::cout << "    -d,--debug               increase debug level"
		<< std::endl;
	std::cout << "    -e,--exposure=<time>     use exposure time <time>"
		<< std::endl;
	std::cout << "    -l,--length=<l>          size of the crop used for "
		"focus" << std::endl;
	std::cout << "    -m,--module<module>      use module named <module>"
		<< std::endl;
	std::cout << "    -h,-?,--help             show this help message and "
		"exit" << std::endl;
}

static struct option	longopts[] = {
{ "camera",	required_argument,	NULL,	'C' }, /* 0 */
{ "ccd",	required_argument,	NULL,	'c' }, /* 1 */
{ "debug",	no_argument,		NULL,	'd' }, /* 2 */
{ "exposure",	required_argument,	NULL,	'e' }, /* 3 */
{ "length",	required_argument,	NULL,	'l' }, /* 4 */
{ "module",	required_argument,	NULL,	'm' }, /* 5 */
{ "help",	no_argument,		NULL,	'h' }, /* 5 */
{ NULL,		0,			NULL,	 0  }, /* 6 */
};

/**
 * \brief main function for the focus program
 */
int	main(int argc, char *argv[]) {
	int	c;
	int	longindex;
	double	exposuretime = 0.1;
	unsigned int	cameraid = 0;
	unsigned int	ccdid = 0;
	int	length = 512;
	std::string	cameratype("uvc");

	while (EOF != (c = getopt_long(argc, argv, "de:m:c:C:l:",
		longopts, &longindex)))
		switch (c) {
		case 'C':
			cameraid = atoi(optarg);
			break;
		case 'c':
			ccdid = atoi(optarg);
			break;
		case 'd':
			debuglevel = LOG_DEBUG;
			break;
		case 'e':
			exposuretime = atof(optarg);
			break;
		case 'l':
			length = atoi(optarg);
			break;
		case 'm':
			cameratype = std::string(optarg);
			break;
		case 'h':
			usage(argv[0]);
			return EXIT_SUCCESS;
		}

	// get the camera
	Repository	repository;
	debug(LOG_DEBUG, DEBUG_LOG, 0, "loading module %s",
		cameratype.c_str());
	ModulePtr	module = repository.getModule(cameratype);
	module->open();


	// get the camera
	DeviceLocatorPtr	locator = module->getDeviceLocator();
	std::vector<std::string>	cameras = locator->getDevicelist();
	if (0 == cameras.size()) {
		std::cerr << "no cameras found" << std::endl;
		return EXIT_FAILURE;
	}
	if (cameraid >= cameras.size()) {
		std::string	msg = stringprintf("camera %d out of range",
			cameraid);
		debug(LOG_ERR, DEBUG_LOG, 0, "%s\n", msg.c_str());
		throw std::range_error(msg);
	}
	std::string	cameraname = cameras[cameraid];
	CameraPtr	camera = locator->getCamera(cameraname);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "camera loaded: %s", cameraname.c_str());

	// get the ccd
	CcdPtr	ccd = camera->getCcd(ccdid);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "get a ccd: %s",
		ccd->getInfo().toString().c_str());

	// get a centerd length x length frame
	ImageSize	framesize(length, length);
	ImageRectangle	frame = ccd->getInfo().centeredRectangle(framesize);
	Exposure	exposure(frame, exposuretime);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "exposure prepared: %s",
		exposure.toString().c_str());

	// retrieve an image
	ccd->startExposure(exposure);
	ImagePtr	image = ccd->getImage();

	// write image
	unlink("test.fits");
	FITSout	out("test.fits");
	out.write(image);

	// apply a mask to keep the border out
	CircleFunction	circle(ImagePoint(length/2, length/2), length/2, 0.8);
	mask(circle, image);
	unlink("masked.fits");
	FITSout	maskout("masked.fits");
	maskout.write(image);

#if 0
	// compute the FOM
	double	fom = focusFOM(image, true,
		Subgrid(ImagePoint(1, 0), ImageSize(1, 1)));
	std::cout << "FOM: " << fom << std::endl;
#endif


	return EXIT_SUCCESS;
}

} // namespace focus
} // namespace app
} // namespace astro

int	main(int argc, char *argv[]) {
	return astro::main_function<astro::app::focus::main>(argc, argv);
}
