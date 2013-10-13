/*
 * list.cpp -- list commands
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <guidecli.h>
#include <listcommand.h>
#include <AstroDebug.h>

namespace astro {
namespace cli {

void	listcommand::operator()(const std::string& command,
		const std::vector<std::string>& arguments) {
	if (arguments.size() == 0) {
		throw command_error("list command requires arguments");
	}
	if (arguments[0] == std::string("modules")) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "list modules command");
		listmodules();
		return;
	}
	throw command_error("cannot execute list command");
}

void	listcommand::listmodules() {
	// get the modules object
	guidesharedcli	gcli;
	Astro::Modules::ModuleNameSequence_var	namelist
		= gcli->modules->getModuleNames();
	for (int i = 0; i < (int)namelist->length(); i++) {
		std::cout << namelist[i] << std::endl;
	}
}

} // namespace cli
} // namespace astro
