/*
 * snowstar.cpp -- main program for the snow star server
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <Ice/Ice.h>
#include <Ice/Properties.h>
#include <Ice/Initialize.h>
#include <cstdlib>
#include <iostream>
#include <DevicesI.h>
#include <ImagesI.h>
#include <AstroDebug.h>
#include <DeviceServantLocator.h>
#include <ImageLocator.h>
#include <AstroTask.h>
#include <TaskQueueI.h>
#include <TaskLocator.h>
#include <GuiderFactoryI.h>
#include <GuiderLocator.h>
#include <ModulesI.h>
#include <DriverModuleLocator.h>
#include <DeviceLocatorLocator.h>
#include <FocusingFactoryI.h>
#include <FocusingLocator.h>
#include <AstroConfig.h>
#include <repository.h>
#include <RepositoriesI.h>
#include <RepositoryLocator.h>
#include <ServiceDiscovery.h>
#include <AstroFormat.h>
#include <InstrumentLocator.h>
#include <InstrumentsI.h>

namespace snowstar {

static struct option	longopts[] = {
{ "base",		required_argument,	NULL,	'b' }, /* 0 */
{ "config",		required_argument,	NULL,	'c' }, /* 1 */
{ "debug",		no_argument,		NULL,	'd' }, /* 2 */
{ "database",		required_argument,	NULL,	'q' }, /* 3 */
{ "sslport",		required_argument,	NULL,	's' }, /* 4 */
{ "name",		required_argument,	NULL,	'n' }, /* 5 */
{ NULL,			0,			NULL,	 0  }, /* 6 */
};

class service_location {
public:
	std::string	servicename;
	unsigned short	port;
	unsigned short	sslport;
	bool	ssl;
	service_location() : port(0), sslport(0), ssl(false) { }
	void	locate();
};

void	service_location::locate() {
	astro::config::ConfigurationPtr	config
		= astro::config::Configuration::get();
	if (servicename.size() == 0) {
		if (config->hasglobal("service", "name")) {
			servicename = config->global("service", "name");
		} else {
			char	h[1024];
			if (gethostname(h, sizeof(h)) < 0) {
				std::string	msg = astro::stringprintf(
					"cannot figure out host name: %s",
					strerror(errno));
				debug(LOG_ERR, DEBUG_LOG, 0, "%s", msg.c_str());
				throw std::runtime_error(msg);
			}
			servicename = std::string(h);
		}
	}
	if (port == 0) {
		if (config->hasglobal("service", "port")) {
			std::string	s = config->global("service", "port");
			port = std::stoi(s);
		} else {
			port = 10000;
		}
	}
	if (sslport == 0) {
		if (config->hasglobal("service", "sslport")) {
			std::string s = config->global("service", "sslport");
			sslport = std::stoi(s);
		}
	}
	ssl = (sslport > 0);
}

/**
 * \brief Main function for the Snowstar server
 */
int	snowstar_main(int argc, char *argv[]) {
	// default debug settings
	debugtimeprecision = 3;
	debugthreads = true;

	// resturn status
	int	status = EXIT_SUCCESS;

	// get properties from the command line
	Ice::PropertiesPtr	props;
	Ice::CommunicatorPtr	ic;
	try {
		props = Ice::createProperties(argc, argv);
		props->setProperty("Ice.MessageSizeMax", "65536"); // 64 MB
		props->setProperty("Ice.Plugin.IceSSL", "IceSSL:createIceSSL");
		Ice::InitializationData	id;
		id.properties = props;
		ic = Ice::initialize(id);
	} catch (...) {
		throw;
	}

	// default configuration
	std::string	databasefile("testdb.db");
	std::string	servicename("");

	// port numbers
	service_location	location;

	// parse the command line
	int	c;
	int	longindex;
	while (EOF != (c = getopt_long(argc, argv, "b:c:dn:p:q:s:",
		longopts, &longindex)))
		switch (c) {
		case 'b':
			astro::image::ImageDirectory::basedir(optarg);
			break;
		case 'c':
			astro::config::Configuration::set_default(optarg);
			break;
		case 'd':
			debuglevel = LOG_DEBUG;
			break;
		case 'q':
			databasefile = std::string(optarg);
			break;
		case 'p':
			location.port = std::stoi(optarg);
			break;
		case 's':
			location.sslport = std::stoi(optarg);
			break;
		case 'n':
			location.servicename = std::string(optarg);
			break;
		}

	// determine which service name to use
	location.locate();
	astro::discover::ServicePublisherPtr	sp
		= astro::discover::ServicePublisher::get(location.servicename,
			location.port);
	astro::discover::ServicePublisherPtr	sps;
	if (location.ssl) {
		sps = astro::discover::ServicePublisher::get(
			location.servicename + "-ssl", location.sslport);
	}

	// set up the repository
	astro::module::Repository	repository;
	astro::module::Devices	devices(repository);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "devices set up");
	sp->set(astro::discover::ServiceSubset::INSTRUMENTS);
	if (sps) { sps->set(astro::discover::ServiceSubset::INSTRUMENTS); }

	// create image directory
	astro::image::ImageDirectory	imagedirectory;
	sp->set(astro::discover::ServiceSubset::IMAGES);
	if (sps) { sps->set(astro::discover::ServiceSubset::IMAGES); }

	// create database and task queue
	astro::persistence::DatabaseFactory	dbfactory;
	astro::persistence::Database	database
		= dbfactory.get(databasefile);
	astro::task::TaskQueue	taskqueue(database);
	sp->set(astro::discover::ServiceSubset::TASKS);
	if (sps) { sps->set(astro::discover::ServiceSubset::TASKS); }

	// create guider factory
	astro::guiding::GuiderFactory	guiderfactory(repository, database);
	sp->set(astro::discover::ServiceSubset::GUIDING);
	if (sps) { sps->set(astro::discover::ServiceSubset::GUIDING); }

	// publish the service name
	sp->publish();
	if (sps) { sps->publish(); }

	// initialize servant
	try {
		// create the adapter
		std::string	connectstring = astro::stringprintf(
			"default -p %hu", location.port);
		if (location.sslport > 0) {
			connectstring += astro::stringprintf(" -p %hu:ssl",
				location.sslport);
		}
		Ice::ObjectAdapterPtr	adapter
			= ic->createObjectAdapterWithEndpoints("Astro",
				connectstring);
		debug(LOG_DEBUG, DEBUG_LOG, 0, "adapters created");

		// add a servant for devices to the device adapter
		Ice::ObjectPtr	object = new DevicesI(devices);
		adapter->add(object, ic->stringToIdentity("Devices"));
		DeviceServantLocator	*deviceservantlocator
			= new DeviceServantLocator(repository, imagedirectory);
		adapter->addServantLocator(deviceservantlocator, "");
		debug(LOG_DEBUG, DEBUG_LOG, 0, "devices servant added");

		// add a servant for images to the adapter
		object = new ImagesI(imagedirectory);
		adapter->add(object, ic->stringToIdentity("Images"));
		ImageLocator	*imagelocator
			= new ImageLocator(imagedirectory);
		adapter->addServantLocator(imagelocator, "image");
		debug(LOG_DEBUG, DEBUG_LOG, 0, "images servant locator added");

		// add a servant for taskqueue to the adapter
		object = new TaskQueueI(taskqueue);
		adapter->add(object, ic->stringToIdentity("Tasks"));
		TaskLocator	*tasklocator = new TaskLocator(database);
		adapter->addServantLocator(tasklocator, "task");
		debug(LOG_DEBUG, DEBUG_LOG, 0, "task locator added");

		// add a servant for the guider factory
		GuiderLocator	*guiderlocator = new GuiderLocator();
		object = new GuiderFactoryI(database, guiderfactory,
			guiderlocator, imagedirectory);
		adapter->add(object, ic->stringToIdentity("Guiders"));
		adapter->addServantLocator(guiderlocator, "guider");

		// add a servant for the modules
		object = new ModulesI();
		adapter->add(object, ic->stringToIdentity("Modules"));
		DriverModuleLocator	*drivermodulelocator
			= new DriverModuleLocator(repository);
		adapter->addServantLocator(drivermodulelocator, "drivermodule");

		// add servant locator for device locator
		DeviceLocatorLocator	*devicelocatorlocator
			= new DeviceLocatorLocator(repository);
		adapter->addServantLocator(devicelocatorlocator,
				"devicelocator");
		debug(LOG_DEBUG, DEBUG_LOG, 0, "Modules servant added");

		// add a servant for Focusing
		object = new FocusingFactoryI();
		adapter->add(object, ic->stringToIdentity("FocusingFactory"));
		FocusingLocator	*focusinglocator = new FocusingLocator();
		adapter->addServantLocator(focusinglocator, "focusing");
		debug(LOG_DEBUG, DEBUG_LOG, 0, "Focusing servant added");

		// add a servant for Repositories
		object = new RepositoriesI();
		adapter->add(object, ic->stringToIdentity("Repositories"));
		debug(LOG_DEBUG, DEBUG_LOG, 0, "Repositories servant added");
		RepositoryLocator	*repolocator = new RepositoryLocator();
		adapter->addServantLocator(repolocator, "repository");
		debug(LOG_DEBUG, DEBUG_LOG, 0, "Repository servant added");

		// add a servant for Instruments
		object = new InstrumentsI();
		adapter->add(object, ic->stringToIdentity("Instruments"));
		debug(LOG_DEBUG, DEBUG_LOG, 0, "Instruments servant added");
		InstrumentLocator	*instrumentlocator = new InstrumentLocator();
		adapter->addServantLocator(instrumentlocator, "instrument");
		debug(LOG_DEBUG, DEBUG_LOG, 0, "Instrument servant added");

		// activate the adapter
		adapter->activate();
		ic->waitForShutdown();
	} catch (const Ice::Exception& ex) {
		std::cerr << "ICE exception: " << ex.what() << std::endl;
		status = EXIT_FAILURE;
	} catch (const char *msg) {
		std::cerr << msg << std::endl;
		status = EXIT_FAILURE;
	}

	// destroy the communicator
	if (ic) {
		ic->destroy();
	}
	return status;
}

} // namespace snowstar

int	main(int argc, char *argv[]) {
	return astro::main_function<snowstar::snowstar_main>(argc, argv);
}
