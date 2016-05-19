/*
 * snowservers.cpp -- program to scan for servers
 *
 * (c) 2016 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <AstroUtils.h>
#include <AstroDiscovery.h>
#include <includes.h>

using namespace astro::discover;

namespace snowstar {
namespace app {
namespace servers {

static struct option	longopts[] = {
{ "debug",	no_argument,		NULL,		'd' },
{ "help",	no_argument,		NULL,		'h' },
{ NULL,		0,			NULL,		0   }
};

static void	usage(const char *progname) {
	astro::Path	path(progname);
	std::string	p = std::string("    ") + path.basename();
	std::cout << "Usage:" << std::endl;
	std::cout << std::endl;
	std::cout << p << " [ options ] " << std::endl;
	std::cout << std::endl;
	std::cout << "list all servers that offer astro photo services";
	std::cout << std::endl;
	std::cout << std::endl;
	std::cout << "Options:" << std::endl;
	std::cout << " -d,--debug         increase debug level" << std::endl;
	std::cout << " -h,--help          display help message and exit"
		<< std::endl;
	std::cout << " -s,--server=<s>    connect to server named <s>, default "
		"is localhost" << std::endl;
	std::cout << std::endl;

}

int	main(int argc, char *argv[]) {
	debug_set_ident("snowservers");
	int	c;
	int	longindex;
	while (EOF != (c = getopt_long(argc, argv, "dh", longopts,
		&longindex))) {
		switch (c) {
		case 'd':
			debuglevel = LOG_DEBUG;
			break;
		case 'h':
			usage(argv[0]);
			return EXIT_SUCCESS;
		}
	}

	// create a service discover object
	ServiceDiscoveryPtr	sd = ServiceDiscovery::get();
	sd->start();

	// find the service keys
	ServiceDiscovery::ServiceKeySet	keys;
	do {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "displaying the list");
		ServiceDiscovery::ServiceKeySet	sks = sd->list();
		debug(LOG_DEBUG, DEBUG_LOG, 0, "%d keys", sks.size());

		ServiceDiscovery::ServiceKeySet	removedkeys;
		std::set_difference(keys.begin(), keys.end(),
			sks.begin(), sks.end(),
			inserter(removedkeys, removedkeys.begin()));

		std::for_each(removedkeys.begin(), removedkeys.end(),
			[sd](const ServiceKey& k) {
				std::cout << "deleted: ";
				std::cout << k.toString();
				std::cout << std::endl;
			}
		);

		ServiceDiscovery::ServiceKeySet	newkeys;
		std::set_difference(sks.begin(), sks.end(),
			keys.begin(), keys.end(),
			inserter(newkeys, newkeys.begin()));
		
		std::for_each(newkeys.begin(), newkeys.end(),
			[sd](const ServiceKey& k) {
				ServiceObject	so = sd->find(k);
				std::cout << so.toString();
				std::cout << " ";
				std::cout << so.ServiceSubset::toString();
				std::cout << std::endl;
			}
		);

		keys = sks;

		sleep(1);
	} while (1);

	return EXIT_SUCCESS;
}

} // namespace servers
} // namespace app
} // namespace snowstar

int	main(int argc, char *argv[]) {
	return astro::main_function<snowstar::app::servers::main>(argc, argv);
}

