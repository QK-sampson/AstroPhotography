/*
 * AstroConfig.h -- classes for configuration management of the AP application
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef _AstroConfig_h
#define _AstroConfig_h

#include <AstroPersistence.h>
#include <AstroDevice.h>
#include <memory>

namespace astro {

// we need to declar the test class, which happens to be in a different
// namespace, because we want to give it access to the internals of the class
namespace test {
class DeviceMapperTest;
} // namespace test

namespace config {

/**
 * \brief Device mapping entry
 */
class DeviceMap {
	std::string	_name;
	DeviceName	_devicename;
	std::string	_servername;
	std::string	_description;
public:
	DeviceMap(const DeviceName& devicename) : _devicename(devicename) { }

	const std::string&	name() const { return _name; }
	void	name(const std::string& n) { _name = n; }

	const std::string&	servername() const { return _servername; }
	void	servername(const std::string& s) { _servername = s; }

	const std::string&	description() const { return _description; }
	void	description(const std::string& d) { _description = d; }

	const DeviceName&	devicename() const { return _devicename; }
	void	devicename(const DeviceName& d) { _devicename = d; }
};

class DeviceMapper;
typedef std::shared_ptr<DeviceMapper>	DeviceMapperPtr;

class Configuration;

/**
 * \brief  Device Mapper class
 */
class DeviceMapper {
static DeviceMapperPtr	get(astro::persistence::Database database);
	DeviceMap	select(const std::string& condition);
public:
	virtual DeviceMap	find(const std::string& name) = 0;
	virtual DeviceMap	find(const DeviceName& devicename,
					const std::string& servername) = 0;
	virtual void	add(const DeviceMap& devicemap) = 0;
	virtual void	update(const std::string& name,
				const DeviceMap& devicemap) = 0;
	virtual void	update(const DeviceName& devicename,
				const std::string& servername,
				const DeviceMap& devicemap) = 0;
	virtual void	remove(const std::string& name) = 0;
	virtual void	remove(const DeviceName& devicename,
				const std::string& servername) = 0;
	friend class astro::test::DeviceMapperTest;
	friend class Configuration;
};

typedef std::shared_ptr<Configuration>	ConfigurationPtr;

/**
 * \brief Configuration repository class
 *
 * All configuration information can be accessed through this interface
 */
class Configuration {
private: // prevent copying of configuration
	Configuration(const Configuration& other);
	Configuration&	operator=(const Configuration& other);
public:
	Configuration() { }
	// factory methods for the configuration
static ConfigurationPtr	get();
static ConfigurationPtr	get(const std::string& filename);
	// global configuration variables
	virtual std::string	global(const std::string& section,
				const std::string& name) = 0;
	virtual std::string	global(const std::string& section,
				const std::string& name,
				const std::string& def) = 0;
	virtual void	setglobal(const std::string& section,
			const std::string& name, const std::string& value) = 0;
	virtual void	removeglobal(const std::string& section,
				const std::string& name) = 0;
};

} // namespace config
} // namespace astro

#endif /* _AstroConfig_h */
