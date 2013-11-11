/*
 * AstroLocator.h -- DeviceLocator
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef _AstroLocator_h
#define _AstroLocator_h

#include <AstroCamera.h>
#include <map>
#include <stdexcept>

namespace astro {
namespace device {

/**
 * \brief Device cache adapter serving devices of any type
 *
 * 
 */
class DeviceLocator;

template<typename Device>
class DeviceCacheAdapter {
	DeviceLocator	*_locator;
public:
	DeviceCacheAdapter(DeviceLocator *locator) : _locator(locator) { }
	typename Device::sharedptr	get0(const DeviceName& name);
};

template<>
astro::camera::CameraPtr
DeviceCacheAdapter<astro::camera::Camera>::get0(const DeviceName& name);

template<>
astro::camera::CcdPtr
DeviceCacheAdapter<astro::camera::Ccd>::get0(const DeviceName& name);

template<>
astro::camera::GuiderPortPtr
DeviceCacheAdapter<astro::camera::GuiderPort>::get0(const DeviceName& name);

template<>
astro::camera::FilterWheelPtr
DeviceCacheAdapter<astro::camera::FilterWheel>::get0(const DeviceName& name);

template<>
astro::camera::CoolerPtr
DeviceCacheAdapter<astro::camera::Cooler>::get0(const DeviceName& name);

template<>
astro::camera::FocuserPtr
DeviceCacheAdapter<astro::camera::Focuser>::get0(const DeviceName& name);

/**
 * \brief Cache for devices
 *
 * Devices are cached by name. This template implements a cache for
 * each type of object.
 */
template<typename Device>
class DeviceCache {
	std::map<std::string, typename Device::sharedptr>	_cache;
	DeviceLocator	*_locator;
public:
	DeviceCache(DeviceLocator *locator) : _locator(locator) { }
	typename Device::sharedptr	get(const std::string& name);
};

template<typename Device>
typename Device::sharedptr	DeviceCache<Device>::get(const std::string& name) {
	DeviceName	devname(name);
	if (!devname.hasType(Device::devicetype)) {
		std::string	t = DeviceName::type2string(Device::devicetype);
		debug(LOG_ERR, DEBUG_LOG, 0, "%s is not of type %s",
			name.c_str(), t.c_str());
		throw std::invalid_argument("name does not refer the "
			"right device type");
	}
	typename Device::sharedptr	device;
	if (_cache.find(name) == _cache.end()) {
		DeviceCacheAdapter<Device>	dca(_locator);
		device = dca.get0(name);
		_cache.insert(std::make_pair(name, device));
	} else {
		device = _cache.find(name)->second;
	}
	return device;
}

/**
 * \brief The device locator can locate device within a module
 *
 * The device locator keeps a cache of all devices ever retrieved from this
 * locator, which ensures that 
 */
class   DeviceLocator {
	DeviceCache<astro::camera::Camera>	cameracache;
	DeviceCache<astro::camera::Ccd>		ccdcache;
	DeviceCache<astro::camera::GuiderPort>	guiderportcache;
	DeviceCache<astro::camera::FilterWheel>	filterwheelcache;
	DeviceCache<astro::camera::Cooler>	coolercache;
	DeviceCache<astro::camera::Focuser>	focusercache;
public:
	virtual	astro::camera::CameraPtr	getCamera0(const DeviceName& name);
	virtual astro::camera::CcdPtr		getCcd0(const DeviceName& name);
	virtual	astro::camera::GuiderPortPtr	getGuiderPort0(const DeviceName& name);
	virtual	astro::camera::FilterWheelPtr	getFilterWheel0(const DeviceName& name);
	virtual	astro::camera::CoolerPtr	getCooler0(const DeviceName& name);
	virtual	astro::camera::FocuserPtr	getFocuser0(const DeviceName& name);
public:
	DeviceLocator();
	virtual ~DeviceLocator();
	virtual std::string	getName() const;
	virtual std::string	getVersion() const;
	virtual std::vector<std::string>	getDevicelist(
		const DeviceName::device_type device = DeviceName::Camera);
	astro::camera::CameraPtr	getCamera(const std::string& name);
	astro::camera::CameraPtr	getCamera(size_t index);
	astro::camera::CcdPtr		getCcd(const std::string& name);
	astro::camera::GuiderPortPtr	getGuiderPort(const std::string& name);
	astro::camera::FilterWheelPtr	getFilterWheel(const std::string& name);
	astro::camera::CoolerPtr	getCooler(const std::string& name);
	astro::camera::FocuserPtr	getFocuser(const std::string& name);
};

typedef std::shared_ptr<DeviceLocator>	DeviceLocatorPtr;

//////////////////////////////////////////////////////////////////////
// adapter class for DeviceLocator to extract objects of a given type
//////////////////////////////////////////////////////////////////////
template<typename device>
class LocatorAdapter {
        astro::device::DeviceLocatorPtr _locator;
public:
        LocatorAdapter(astro::device::DeviceLocatorPtr locator)
                : _locator(locator) { }
        typename device::sharedptr      get(const std::string& name);
};

template<>
astro::camera::CameraPtr	LocatorAdapter<astro::camera::Camera>::get(
					const std::string& name);

template<>
astro::camera::CcdPtr	LocatorAdapter<astro::camera::Ccd>::get(
					const std::string& name);

template<>
astro::camera::GuiderPortPtr	LocatorAdapter<astro::camera::GuiderPort>::get(
					const std::string& name);

template<>
astro::camera::FilterWheelPtr	LocatorAdapter<astro::camera::FilterWheel>::get(
					const std::string& name);

template<>
astro::camera::CoolerPtr	LocatorAdapter<astro::camera::Cooler>::get(
					const std::string& name);

template<>
astro::camera::FocuserPtr	LocatorAdapter<astro::camera::Focuser>::get(
					const std::string& name);

} // namespace device
} // namespace astro

#endif /* _AstroLocator_h */


