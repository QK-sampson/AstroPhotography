/*
 * fitsheader.cpp -- fits header manipulation utility
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <stdexcept>
#include <includes.h>
#include <AstroDebug.h>
#include <AstroFormat.h>
#include <iostream>
#include <fitsio.h>
#include <string>

namespace astro {

/**
 * \brief Display all headers of a fits file
 */
void	display_headers(fitsfile *fits) {
	int	status = 0;
	int	keynum = 1;
	char	keyname[100];
	char	value[100];
	char	comment[100];
	while (1) {
		if (fits_read_keyn(fits, keynum, keyname, value, comment,
			&status)) {
			return;
		}

		// display the header just found:
		std::cout << stringprintf("%-8.8s = %s / %s",
			keyname, value, comment) << std::endl;
		keynum++;
	}
	debug(LOG_DEBUG, DEBUG_LOG, 0, "headers displayed");
}

/**
 * \brief Delete a header from a FITS file
 */
void	delete_header(fitsfile *fits, const char *headername) {
	int	status = 0;
	char	fitserrmsg[80];
	if (fits_delete_key(fits, headername, &status)) {
		fits_get_errstatus(status, fitserrmsg);
		std::string	msg = stringprintf("cannot delete header: %s",
			fitserrmsg);
		throw std::runtime_error(msg);
	}
	debug(LOG_DEBUG, DEBUG_LOG, 0, "header '%s' deleted", headername);
}

/**
 * \brief Add a header to a FITS file
 */
void	add_header(fitsfile *fits, const char *key, const char *value,
		const char *comment) {
	int	status = 0;
	char	fitserrmsg[80];

	// first try integer type
	try {
		long	ivalue = std::stol(std::string(value));
		double	dvalue = std::stod(std::string(value));

		if (ivalue == dvalue) {
			if (fits_write_key(fits, TLONG, (char *)key,
				(char *)&ivalue, (char *)comment, &status)) {
				fits_get_errstatus(status, fitserrmsg);
				std::string msg = stringprintf(
					"cannot add '%s': %s", key, fitserrmsg);
				throw std::runtime_error(msg);
			}
			debug(LOG_DEBUG, DEBUG_LOG, 0, "added int header");
		} else {
			if (fits_write_key(fits, TDOUBLE, (char *)key,
				(char *)&dvalue, (char *)comment, &status)) {
				fits_get_errstatus(status, fitserrmsg);
				std::string msg = stringprintf(
					"cannot add '%s': %s", key, fitserrmsg);
				throw std::runtime_error(msg);
			}
			debug(LOG_DEBUG, DEBUG_LOG, 0, "added double header");
		}
		return;
	} catch (...) {
	}

	// if either failed, add as string header
	if (fits_write_key(fits, TSTRING, (char *)key, (char *)value,
		(char *)comment, &status)) {
		fits_get_errstatus(status, fitserrmsg);
		std::string	msg = stringprintf("cannot add '%s': %s",
					key, fitserrmsg);
		throw std::runtime_error(msg);
	}
	debug(LOG_DEBUG, DEBUG_LOG, 0, "header '%s' added", key);
}

int	main(int argc, char *argv[]) {
	int	c;
	while (EOF != (c = getopt(argc, argv, "d")))
		switch (c) {
		case 'd':
			debuglevel = LOG_DEBUG;
			break;
		}

	// next argument must be the command
	if (optind >= argc) {
		throw std::runtime_error("not enough arguments");
	}
	std::string	command(argv[optind++]);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "command: %s", command.c_str());

	// find out whether we need to write the file
	bool	readonly = false;
	if (command == "display") {
		readonly = true;
	}

	// next argument must be the image file
	if (optind >= argc) {
		throw std::runtime_error("not enough arguments");
	}
	std::string	filename(argv[optind++]);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "filename: %s", filename.c_str());

	// open the FITS file
	char	fitserrmsg[80];
	int	status = 0;
	fitsfile	*fits = NULL;
	if (fits_open_file(&fits, filename.c_str(),
		(readonly) ? READONLY : READWRITE, &status)) {
		fits_get_errstatus(status, fitserrmsg);
		std::string	msg = stringprintf("FITS error: %s",
					fitserrmsg);
		throw std::runtime_error(msg);
	}
	debug(LOG_DEBUG, DEBUG_LOG, 0, "file opened");

	// execute the command
	if (command == "display") {
		display_headers(fits);
	}
	if (command == "delete") {
		while (optind < argc) {
			delete_header(fits, argv[optind++]);
		}
	}
	if (command == "add") {
		while (optind + 2 < argc) {
			add_header(fits, argv[optind], argv[optind + 1],
				argv[optind + 2]);
			optind += 3;
		}
	}

	// close the file again
	fits_close_file(fits, &status);

	return EXIT_SUCCESS;
}

} // namespace astro

int	main(int argc, char *argv[]) {
	try {
		return astro::main(argc, argv);
	} catch (std::exception& x) {
		std::cerr << "fitsheader terminated by exception: ";
		std::cerr << x.what() << std::endl;
	} catch (...) {
		std::cerr << "fitsheader terminated by unknown exception";
		std::cerr << std::endl;
	}
}


