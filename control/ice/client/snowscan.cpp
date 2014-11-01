/*
 * snowscan.cpp -- command line tool to scan server for devices
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <stdexcept>
#include <Ice/Ice.h>
#include <iostream>
#include <AstroUtils.h>
#include <AstroFormat.h>
#include <includes.h>
#include <device.h>
#include <CommunicatorSingleton.h>

using namespace snowstar;

namespace snowstar {
namespace app {
namespace snowscan {

/**
 * \brief list of options for the snowscan command
 */
static struct option	longopts[] = {
{ "debug",	no_argument,		NULL,		'd' }, /* 1 */
{ "help",	no_argument,		NULL,		'h' }, /* 2 */
{ "server",	required_argument,	NULL,		's' }, /* 4 */
{ NULL,		0,			NULL,		0   }
};

/**
 * \brief Usage display method
 */
void	usage(const char *progname) {
	astro::Path	path(progname);
	std::string	p = std::string("    ") + path.basename();
	std::cout << "Usage:" << std::endl;
	std::cout << std::endl;
	std::cout << p << " [ options ] modules" << std::endl;
	std::cout << std::endl;
	std::cout << "List all modules available on the server" << std::endl;
	std::cout << std::endl;
	std::cout << p << " [ options ] scan <modulename> " << std::endl;
	std::cout << std::endl;
	std::cout << "Retrieve the devices available from the module name "
		"<modulename>." << std::endl;
	std::cout << "The devices are grouped by type." << std::endl;
	std::cout << std::endl;
	std::cout << p << " [ options ] help" << std::endl;
	std::cout << std::endl;
	std::cout << "Display help about the snowscan program." << std::endl;
	std::cout << std::endl;
	std::cout << "Options:" << std::endl;
	std::cout << " -d,--debug         increase debug level" << std::endl;
	std::cout << " -h,--help          display help message and exit"
		<< std::endl;
	std::cout << " -s,--server=<s>    connect to server named <s>, default "
		"is localhost" << std::endl;
	std::cout << std::endl;
}

/**
 * \brief Implementation of the modules command
 */
int	command_modules(snowstar::ModulesPrx& modules) {
	std::cout << "number of modules: " << modules->numberOfModules();
	std::cout << std::endl;
	std::cout << "modules names:" ;
	snowstar::ModuleNameList	names = modules->getModuleNames();
	std::for_each(names.begin(), names.end(), [](const std::string& name) {
			std::cout << " " << name;
		}
	);
	std::cout << std::endl;
	return EXIT_SUCCESS;
}

/**
 * \brief implementation of the help command
 */
int	command_help(const char *progname) {
	usage(progname);
	return EXIT_SUCCESS;
}


/**
 * \brief List elements of a certain type
 */
void	devicelist(snowstar::DeviceLocatorPrx devicelocator,
	snowstar::devicetype type) {
	snowstar::DeviceNameList	devicenames
		= devicelocator->getDevicelist(type);
	std::for_each(devicenames.begin(), devicenames.end(),
		[](const std::string& name) {
			 std::cout << " " << name;
		}
	);
	std::cout << std::endl;
}

/**
 * \brief Implementation of the scan command
 */
int	command_scan(snowstar::ModulesPrx& modules,
		const std::string& modulename) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "scanning module %s",
		modulename.c_str());
	std::cout << "scanning module " << modulename << std::endl;
	snowstar::DriverModulePrx	module
		= modules->getModule(modulename);
	std::cout << "name:     " << module->getName() << std::endl;
	std::cout << "version:  " << module->getVersion() << std::endl;
	std::cout << "locator:  ";
	std::cout << ((module->hasLocator()) ? "YES" : "NO");
	std::cout << std::endl;
	if (!module->hasLocator()) {
		return EXIT_SUCCESS;
	}
	snowstar::DeviceLocatorPrx	devicelocator
		= module->getDeviceLocator();
	snowstar::DeviceNameList	devicenames;
	std::cout << "cameras:      ";
	devicelist(devicelocator, snowstar::DevCAMERA);
	std::cout << "ccds:         ";
	devicelist(devicelocator, snowstar::DevCCD);
	std::cout << "coolers:      ";
	devicelist(devicelocator, snowstar::DevCOOLER);
	std::cout << "filterwheels: ";
	devicelist(devicelocator, snowstar::DevFILTERWHEEL);
	std::cout << "focuser:      ";
	devicelist(devicelocator, snowstar::DevFOCUSER);
	std::cout << "guiderport:   ";
	devicelist(devicelocator, snowstar::DevGUIDERPORT);
	
	return EXIT_SUCCESS;
}

/**
 * \brief Main function for the snowcan program
 */
int	main(int argc, char *argv[]) {
	CommunicatorSingleton cs(argc, argv);
	Ice::CommunicatorPtr	ic = CommunicatorSingleton::get();

	// some variables changed by options
	astro::ServerName	servername("localhost");

	// parse the remaining command line arguments
	int	c;
	int	longindex;
	while (EOF != (c = getopt_long(argc, argv, "dhs:", longopts,
		&longindex))) {
		switch (c) {
		case 'd':
			debuglevel = LOG_DEBUG;
			break;
		case 'h':
			usage(argv[0]);
			return EXIT_SUCCESS;
		case 's':
			servername = astro::ServerName(optarg);
			break;
		}
	}

	// next argument is the module
	if (argc <= optind) {
		throw std::runtime_error("command argument missing");
	}
	std::string	commandname = argv[optind++];

	// help comand
	if ("help" == commandname) {
		return command_help(argv[0]);
	}

	// connect to the server
	Ice::ObjectPrx	base = ic->stringToProxy(servername.connect("Modules"));
	snowstar::ModulesPrx	modules
		= snowstar::ModulesPrx::checkedCast(base);
	if (!modules) {
		throw std::runtime_error("no modules proxy");
	}

	if ("modules" == commandname) {
		return command_modules(modules);
	}

	if ("scan" == commandname) {
		if (optind >= argc) {
			throw std::string("missing module name");
		}
		std::string	modulename = argv[optind++];
		return command_scan(modules, modulename);
	}
	return EXIT_FAILURE;
}
	
} // namespace snowscan
} // namespace app
} // namespace snowstar

int main(int argc, char *argv[]) {
	return astro::main_function<snowstar::app::snowscan::main>(argc, argv);
}
