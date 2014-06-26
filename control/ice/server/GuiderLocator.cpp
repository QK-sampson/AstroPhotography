/*
 * GuiderLocator.cpp -- guider servant locator implementation
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <GuiderLocator.h>
#include <NameConverter.h>
#include <AstroFormat.h>
#include <AstroDebug.h>
#include <exceptions.h>

namespace snowstar {

GuiderLocator::GuiderLocator() {
}

void	GuiderLocator::add(const std::string& name, Ice::ObjectPtr guiderptr) {
	if (guiders.find(name) != guiders.end()) {
		debug(LOG_WARNING, DEBUG_LOG, 0,
			"warning: guider '%s' already in map", name.c_str());
	}
	guiders.insert(std::make_pair(name, guiderptr));
}

Ice::ObjectPtr	GuiderLocator::locate(const Ice::Current& current,
			Ice::LocalObjectPtr& cookie) {
	std::string	guidername = NameConverter::urldecode(current.id.name);
	guidermap::iterator	i = guiders.find(guidername);
	if (guiders.end() == i) {
		std::string	msg = astro::stringprintf("guider '%s' not found",
					guidername.c_str());
		debug(LOG_ERR, DEBUG_LOG, 0, "%s", msg.c_str());
		throw NotFound(msg);
	}
	return i->second;
}

void	GuiderLocator::finished(const Ice::Current& current,
				const Ice::ObjectPtr& servant,
				const Ice::LocalObjectPtr& cookie) {
}

void	GuiderLocator::deactivate(const std::string& category) {
}

} // namespace snowstar