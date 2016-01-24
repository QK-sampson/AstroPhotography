/*
 * snowguide.cpp -- command line client to control guiding
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hohschule Rapperswil
 */
#include <AstroUtils.h>
#include <iostream>
#include <cstdlib>
#include <typeinfo>
#include <getopt.h>
#include <CommunicatorSingleton.h>
#include <AstroConfig.h>
#include <IceConversions.h>
#include "guide.h"

using namespace snowstar;

namespace snowstar {
namespace app {
namespace snowguide {

/**
 * \brief long options for the snow guiding program
 */
static struct option	longopts[] = {
{ "adaptiveoptics",	required_argument,	NULL,	'a' }, /*  0 */
{ "aointerval",		required_argument,	NULL,	'A' },
{ "binning",		required_argument,	NULL,	'b' }, /*  0 */
{ "ccd",		required_argument,	NULL,	'C' }, /*  1 */
{ "config",		required_argument,	NULL,	'c' }, /*  2 */
{ "csv",		no_argument,		NULL,	 1  }, /*  3 */
{ "debug",		no_argument,		NULL,	'd' }, /*  4 */
{ "exposure",		required_argument,	NULL,	'e' }, /*  5 */
{ "guiderport",		required_argument,	NULL,	'G' }, /*  7 */
{ "help",		no_argument,		NULL,	'h' }, /*  8 */
{ "interval",		required_argument,	NULL,	'i' }, /*  9 */
{ "method",		required_argument,	NULL,	'm' }, /* 10 */
{ "prefix",		required_argument,	NULL,	'p' }, /* 11 */
{ "rectangle",		required_argument,	NULL,	'r' }, /* 12 */
{ "star",		required_argument,	NULL,	's' }, /* 13 */
{ "temperature",	required_argument,	NULL,	't' }, /* 14 */
{ "verbose",		no_argument,		NULL,	'v' }, /* 15 */
{ "width",		required_argument,	NULL,	'w' }, /* 16 */
{ NULL,			0,			NULL,    0  }  /* 17 */
};

/**
 * \brief Main program for the snowguide program
 */
int	main(int argc, char *argv[]) {
	CommunicatorSingleton	communicator(argc, argv);

	double	temperature = std::numeric_limits<double>::quiet_NaN();
	std::string	binning;
	std::string	frame;
	Guide	guide;

	int	c;
	int	longindex;
	int	ccdIndex = 0;
	int	guiderportIndex = 0;
	int	adaptiveopticsIndex = 0;
	int	width = -1;

	guide.exposure.exposuretime = 1.;

	while (EOF != (c = getopt_long(argc, argv,
		"A:a:b:c:C:de:f:G:hi:m:r:s:t:vw:", longopts, &longindex)))
		switch (c) {
		case 'a':
			adaptiveopticsIndex = std::stoi(optarg);
			break;
		case 'A':
			guide.aointerval = std::stod(optarg);
			break;
		case 'b':
			binning = optarg;
			break;
		case 'c':
			astro::config::Configuration::set_default(optarg);
			break;
		case 'C':
			ccdIndex = std::stoi(optarg);
			break;
		case 'd':
			debuglevel = LOG_DEBUG;
			break;
		case 'e':
			guide.exposure.exposuretime = std::stod(optarg);
			break;
		case 'G':
			guiderportIndex = std::stoi(optarg);
			break;
		case 'h':
			guide.usage(argv[0]);
			return EXIT_SUCCESS;
		case 'i':
			guide.guideinterval = std::stod(optarg);
			break;
		case 'm': {
			std::string	m(optarg);
			if (m == "star") {
				guide.method = TrackerSTAR;
			} else if (m == "phase") {
				guide.method = TrackerPHASE;
			} else if (m == "diff") {
				guide.method = TrackerDIFFPHASE;
			} else {
				std::string	msg = astro::stringprintf(
					"unkown tracker method: %s", m.c_str());
				debug(LOG_ERR, DEBUG_LOG, 0, "%s", msg.c_str());
				throw std::runtime_error(msg);
			}
			}
			break;
		case 'p':
			guide.prefix = std::string(optarg);
			break;
		case 'r':
			frame = optarg;
			break;
		case 's':
			guide.star = convert(astro::image::ImagePoint(optarg));
			break;
		case 't':
			temperature = std::stod(optarg);
			break;
		case 'v':
			guide.verbose = true;
			break;
		case 'w':
			width = std::stoi(optarg);
			break;
		case 1:
			guide.csv = true;
			break;
		}

	// next argument is the command
	if (argc <= optind) {
		throw std::runtime_error("missing  argument");
		guide.usage(argv[0]);
		return EXIT_FAILURE;
	}
	std::string	argument(argv[optind++]);
	
	// handle simple help argument
	if ("help" == argument) {
		return guide.help_command(argv[0]);
	}

	// if this is not the help command, then we need three more
	// arguments
	astro::ServerName	servername(argument);
	if (argc <= optind) {
		throw std::runtime_error("missing instrument name argument");
	}
	std::string	instrumentname(argv[optind++]);
	if (argc <= optind) {
		throw std::runtime_error("missing command argument");
	}
	std::string	command = argv[optind++];
	debug(LOG_DEBUG, DEBUG_LOG, 0, "command: %s", command.c_str());

	// handle the help command
	if (command == "help") {
		return guide.help_command(argv[0]);
	}

	// server of the instrument
	debug(LOG_DEBUG, DEBUG_LOG, 0, "instrument %s on server %s",
		std::string(instrumentname).c_str(),
		servername.toString().c_str());

	// build the guider descriptor
	GuiderDescriptor	descriptor;
	descriptor.instrumentname = instrumentname;
	descriptor.ccdIndex = ccdIndex;
	descriptor.guiderportIndex = guiderportIndex;
	descriptor.adaptiveopticsIndex = adaptiveopticsIndex;
	debug(LOG_DEBUG, DEBUG_LOG, 0, "instrument: %s",
		descriptor.instrumentname.c_str());
	debug(LOG_DEBUG, DEBUG_LOG, 0, "ccd: %d", descriptor.ccdIndex);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "guiderport: %d",
		descriptor.guiderportIndex);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "adaptiveoptics: %d",
		descriptor.adaptiveopticsIndex);

	// connect to the guider factory of a remote server
	Ice::CommunicatorPtr	ic = CommunicatorSingleton::get();
        Ice::ObjectPrx  gbase
		= ic->stringToProxy(servername.connect("Guiders"));
	GuiderFactoryPrx	guiderfactory
		= GuiderFactoryPrx::checkedCast(gbase);

	// the next action depends on the command to execute. This first
	// group of commands does not need a guider
	if (command == "list") {
		return guide.list_command(guiderfactory, descriptor);
	}
	if (command == "tracks") {
		return guide.tracks_command(guiderfactory, descriptor);
	}
	if (command == "forget") {
		std::list<int>	ids;
		while (optind < argc) {
			ids.push_back(std::stoi(argv[optind++]));
		}
		return guide.forget_command(guiderfactory, ids);
	}
	if (command == "history") {
		if (argc <= optind) {
			throw std::runtime_error("missing history id");
		}
		long	historyid = std::stoi(argv[optind++]);
		if (argc > optind) {
			ControlType	type = ControlGuiderPort;
			std::string	typestring(argv[optind++]);
			if (typestring == "GP") {
				type = ControlGuiderPort;
			}
			if (typestring == "AO") {
				type = ControlAdaptiveOptics;
			}
			return guide.history_command(guiderfactory,
				historyid, type);
		}
		return guide.history_command(guiderfactory, historyid);
	}
	if (command == "trash") {
		std::list<int>	ids;
		while (optind < argc) {
			ids.push_back(std::stoi(argv[optind++]));
		}
		return guide.trash_command(guiderfactory, ids);
	}

	// retrieve a guider
	GuiderPrx	guider = guiderfactory->get(descriptor);
	GuiderState	state = guider->getState();
	debug(LOG_DEBUG, DEBUG_LOG, 0, "found the guider in state %s",
		guiderstate2string(state).c_str());

	// commands needing a guider
	if (command == "state") {
		return guide.state_command(guider);
	}
	if (command == "stop") {
		return guide.stop_command(guider);
	}
	if (command == "repository") {
		if (argc > optind) {
			try {
				std::string	reponame(argv[optind++]);
				debug(LOG_DEBUG, DEBUG_LOG, 0, "repo name: %s",
					reponame.c_str());
				return guide.repository_command(guider, reponame);
			} catch (const NotFound& x) {
				std::cerr << x.cause << std::endl;
				return EXIT_FAILURE;
			}
		} else {
			return guide.repository_command(guider);
		}
	}
	if (command == "calibration") {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "argc = %d, optind = %d",
			argc, optind);
		if (argc > optind) {
			return guide.calibration_command(guiderfactory,
				guider, std::string(argv[optind++]));
		}
		return guide.calibration_command(guiderfactory, guider);
	}
	if (command == "cancel") {
		return guide.cancel_command(guider);
	}
	if ((command == "image") || (command == "images")) {
		if (argc <= optind) {
			throw std::runtime_error("");
		}
		return guide.images_command(guider,
			std::string(argv[optind++]));
	}
	if (command == "monitor") {
		return guide.monitor_command(guider);
	}

	// the guide and calibrate commands need an exposure
	guide.exposure.gain = 1;
	guide.exposure.limit = 0;
	guide.exposure.shutter = ShOPEN;
	guide.exposure.purpose = ExGUIDE;
	if (binning.size() > 0) {
		guide.exposure.mode = convert(astro::camera::Binning(binning));
	} else {
		guide.exposure.mode.x = 1;
		guide.exposure.mode.y = 1;
	}
	if (frame.size() > 0) {
		guide.exposure.frame = convert(
			astro::image::ImageRectangle(frame));
	} else {
		guide.exposure.frame.origin.x = guide.star.x - width / 2;
		guide.exposure.frame.origin.y = guide.star.y - width / 2;
		guide.exposure.frame.size.width = width;
		guide.exposure.frame.size.height = width;
	}
	guider->setExposure(guide.exposure);

	// make sure we have the guide star set
	Point	starpoint;
	starpoint.x = guide.star.x;
	starpoint.y = guide.star.y;
	guider->setStar(starpoint);

	// implement the guide and calibrate commands
	if (command == "guide") {
		return guide.guide_command(guider);
	}
	if (command == "calibrate") {
		// next argument must be the calibration id, if it is present
		if (argc > optind) {
			return guide.calibrate_command(guider,
				std::string(argv[optind++]));
		}
		return guide.calibrate_command(guider, -1);
	}

	std::string	msg = astro::stringprintf("unknown command '%s'",
		command.c_str());
	guide.usage(argv[0]);
	throw std::runtime_error(msg);
}

} // namespace snowguide
} // namespace app
} // namespace snowstar

int	main(int argc, char *argv[]) {
	return astro::main_function<snowstar::app::snowguide::main>(argc, argv);
}
