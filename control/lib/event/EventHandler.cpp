/*
 * EventHandler.cpp -- event handler class implementation
 * 
 * (c) 2016 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <AstroEvent.h>
#include <AstroConfig.h>
#include <AstroDiscovery.h>
#include <AstroDebug.h>
#include <includes.h>
#include <sstream>

using namespace astro::config;
using namespace astro::discover;
using namespace astro::callback;

namespace astro {
namespace events {

#if 0
std::string	level2string(eventlevel_t level) {
	switch (level) {
	case DEBUG:	return std::string("DEBUG");
	case INFO:	return std::string("INFO");
	case NOTICE:	return std::string("NOTICE");
	case WARNING:	return std::string("WARNING");
	case ERR:	return std::string("ERR");
	case CRIT:	return std::string("CRIT");
	case ALERT:	return std::string("ALERT");
	case EMERG:	return std::string("EMERG");
	}
	std::string	msg = stringprintf("unknown level: %d", level);
	debug(LOG_ERR, DEBUG_LOG, 0, "%s", msg.c_str());
	throw std::runtime_error(msg);
}

std::string	Event::subsystem2string(Event::Subsystem s) const {
	switch (s) {
	case DEBUG:		return std::string("debug");
	case DEVICE:		return std::string("device");
	case FOCUS:		return std::string("focus");
	case GUIDE:		return std::string("guide");
	case IMAGE:		return std::string("image");
	case INSTRUMENT:	return std::string("instrument");
	case MODULE:		return std::string("module");
	case REPOSITORY:	return std::string("repository");
	case SERVER:		return std::string("server");
	case TASK:		return std::string("task");
	case UTILITIES:		return std::string("utilities");
	}
	std::string	msg = stringprintf("unknown subsystem %d", s);
	debug(LOG_ERR, DEBUG_LOG, 0, "%s", msg.c_str());
	throw std::runtime_error(msg);
}

Event::Subsystem	Event::string2subsystem(const std::string& s) const {
	if (s == std::string("debug"))		{ return DEBUG; 	}
	if (s == std::string("device"))		{ return DEVICE; 	}
	if (s == std::string("focus"))		{ return FOCUS; 	}
	if (s == std::string("guide"))		{ return GUIDE; 	}
	if (s == std::string("image"))		{ return IMAGE; 	}
	if (s == std::string("instrument"))	{ return INSTRUMENT; 	}
	if (s == std::string("module"))		{ return MODULE; 	}
	if (s == std::string("repository"))	{ return REPOSITORY; 	}
	if (s == std::string("server"))		{ return SERVER; 	}
	if (s == std::string("task"))		{ return TASK; 		}
	if (s == std::string("utilities"))	{ return UTILITIES; 	}
	std::string	msg = stringprintf("unknown subsystem '%s'", s.c_str());
	debug(LOG_ERR, DEBUG_LOG, 0, "%s", msg.c_str());
	throw std::runtime_error(msg);
}

std::string	Event::toString() const {
	std::ostringstream	out;
	out << "level=" << level2string(level) << ", ";
	out << "pid=" << pid << ", ";
	out << "service=" << service << ", ";
	out << "subsystem=" << subsystem << ", ";
	out << "classname=" << classname << ", ";
	out << "file:line=" << file << ":" << line << ", ";
	out << "massage=" << message;
	return out.str();
}
#endif

static EventHandler	handler;

bool	EventHandler::active() {
	return handler._active;
}

void	EventHandler::active(bool a) {
	handler._active = a;
}

void	EventHandler::callback(CallbackPtr c) {
	handler._callback = c;
}

EventHandler&	EventHandler::get() {
	return handler;
}

void	EventHandler::consume(const std::string& file, int line,
		const std::string& classname, eventlevel_t level,
		const Event::Subsystem subsystem,
		const std::string& message) {
	return handler.process(file, line, classname, level, subsystem, message);
}

/**
 * \brief Main event processing method
 */
void	EventHandler::process(const std::string& file, int line,
		const std::string& classname, eventlevel_t level,
		const Event::Subsystem subsystem,
		const std::string& message) {
	if (!_active) {
		return;
	}
	if (!database) {
		database = Configuration::get()->database();
	}
	if (!database) {
		// still no database, give up
		return;
	}
	
	EventRecord	record(-1);
	record.level = level;
	record.pid = getpid();
	record.service = discover::ServiceLocation::get().servicename();
	gettimeofday(&record.eventtime, NULL);
	switch (subsystem) {
	case Event::DEBUG:
		record.subsystem = "debug";
		break;
	case Event::DEVICE:
		record.subsystem = "device";
		break;
	case Event::FOCUS:
		record.subsystem = "focus";
		break;
	case Event::GUIDE:
		record.subsystem = "guide";
		break;
	case Event::IMAGE:
		record.subsystem = "image";
		break;
	case Event::INSTRUMENT:
		record.subsystem = "instrument";
		break;
	case Event::MODULE:
		record.subsystem = "module";
		break;
	case Event::REPOSITORY:
		record.subsystem = "repository";
		break;
	case Event::SERVER:
		record.subsystem = "server";
		break;
	case Event::TASK:
		record.subsystem = "task";
		break;
	case Event::UTILITIES:
		record.subsystem = "utilities";
		break;
	}
	record.message = message;
	record.classname = classname;
	record.file = file;
	record.line = line;

	// save the record in the database table
	EventTable	table(database);
	table.add(record);

	// if a callback is installed, send the event to the callback
	if (!_callback) {
		return;
	}
	EventCallbackData	*ecd = new EventCallbackData(record);
	CallbackDataPtr	cd = CallbackDataPtr(ecd);
	_callback->operator()(cd);
}

} // namespace events

void	event(const char *file, int line, const std::string& classname,
		events::eventlevel_t level,
		const events::Event::Subsystem subsystem,
		const std::string& message) {
	try {
		events::EventHandler::consume(file, line, classname,
			level, subsystem, message);
	} catch (const std::exception& x) {
		debug(LOG_ERR, DEBUG_LOG, 0, "cannot write event: %s",
			x.what());
	} catch (...) {
		debug(LOG_ERR, DEBUG_LOG, 0, "cannot write event");
	}
}

} // namespace astro
