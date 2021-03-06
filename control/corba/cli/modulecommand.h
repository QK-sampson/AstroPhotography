/*
 * modulecommand.h -- command class for module commands
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef _modulecommand_h
#define _modulecommand_h

#include <clicommand.h>
#include <module.hh>

namespace astro {
namespace cli {

class modulecommand : public clicommand {
	void	listdevices(const std::string& modulename,
			const enum Astro::DeviceLocator::device_type devicetype);
	void	moduleversion(const std::string& modulename);
public:
	modulecommand(commandfactory& factory) : clicommand(factory, "module") { }
	~modulecommand() { }
	virtual void	operator()(const std::string& command,
			const std::vector<std::string>& arguments);
	virtual std::string	summary() const;
	virtual std::string	help() const;
};

} // namespace cli
} // namespace astro

#endif /* _modulecommand_h */
