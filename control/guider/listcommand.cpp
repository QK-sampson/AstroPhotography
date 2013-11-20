/*
 * list.cpp -- list commands
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <guidecli.h>
#include <listcommand.h>
#include <AstroDebug.h>
#include <CorbaExceptionReporter.h>

namespace astro {
namespace cli {

void	listcommand::operator()(const std::string& command,
		const std::vector<std::string>& arguments) {
	if (arguments.size() == 0) {
		throw command_error("list command requires arguments");
	}
	if (arguments[0] == std::string("modules")) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "list modules command");
		try {
			listmodules();
		} catch (CORBA::Exception& x) {
			debug(LOG_ERR, DEBUG_LOG, 0,
				"list modules throws exception: %s",
				Astro::exception2string(x).c_str());
		}
		return;
	}
	throw command_error("cannot execute list command");
}

void	listcommand::listmodules() {
	// get the modules object
	guidesharedcli	gcli;
	Astro::Modules::ModuleNameSequence_var	namelist;
	try {
		namelist = gcli->modules->getModuleNames();
	} catch (const CORBA::Exception& x) {
		debug(LOG_ERR, DEBUG_LOG, 0,
			"getModuleNames throws exception: %s",
			Astro::exception2string(x).c_str());
		return;
	}
	for (int i = 0; i < (int)namelist->length(); i++) {
		std::cout << namelist[i] << std::endl;
	}
}

std::string	listcommand::summary() const {
	return std::string("list various object types");
}

std::string	listcommand::help() const {
	return std::string(
		"SYNOPSIS\n"
		"\n"
		"\tlist <type>\n"
		"\n"
		"DESCRIPTION\n"
		"\n"
		"Display a list of objects of a given <type>. Valid <type>\n"
		"values are \"modules\".\n"
	);
}

} // namespace cli
} // namespace astro