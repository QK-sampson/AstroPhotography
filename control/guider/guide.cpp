/*
 * guide.cpp -- guide command line client
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#if _HAVE_CONFIG_H
#include <config.h>
#endif /* _HAVE_CONFIG_H */

#include <stdlib.h>
#include <stdio.h>
#include <AstroDebug.h>
#include <stdexcept>
#include <unistd.h>
#include <iostream>
#include <OrbSingleton.h>
#include <NameService.h>
#include <errno.h>
#include <string.h>
#include <guidecli.h>
#include <module.hh>

extern int	yydebug;

namespace astro {

int	main(int argc, char *argv[]) {
	Astro::OrbSingleton	orb(argc, argv);

	int	c;
	char	*filename = NULL;
	while (EOF != (c = getopt(argc, argv, "df:")))
		switch (c) {
		case 'd':
			debuglevel = LOG_DEBUG;
			break;
		case 'f':
			filename = optarg;
			break;
		}

	debug(LOG_DEBUG, DEBUG_LOG, 0, "guide program started");

	// create the command line interpreter
	astro::cli::guidecli	cli;
	astro::cli::guidesharedcli	s(&cli);

	// get a reference to the naming service
	Astro::Naming::NameService	nameservice(orb);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "got naming service");

	// Next we want to get a reference to the Modules object
	Astro::Naming::Names	names;
	names.push_back(Astro::Naming::Name("Astro", "context"));
	names.push_back(Astro::Naming::Name("Modules", "object"));
	CORBA::Object_var	obj = nameservice.lookup(names);

	// get a reference to the modules interface
	cli.modules = Astro::Modules::_narrow(obj);
	if (CORBA::is_nil(cli.modules)) {
		throw std::runtime_error("nil object reference");
	}
	debug(LOG_DEBUG, DEBUG_LOG, 0, "got a reference to a Modules object");


	/* start parsing the input */
	if (filename) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "parsing '%s'", filename);
		cli.parse(filename);
	} else {
		std::cerr << "cannot read stdin" << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

} // namespace astro

int	main(int argc, char *argv[]) {
	try {
		return astro::main(argc, argv);
	} catch (std::exception& x) {
		std::cerr << "guide program terminated by exception: "
			<< x.what() << std::endl;
	}
	exit(EXIT_FAILURE);
}