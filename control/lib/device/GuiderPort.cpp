/* 
 * GuiderPort.pp -- Guider Port implementation
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <AstroCamera.h>

namespace astro {
namespace camera {

DeviceName::device_type	GuiderPort::devicetype = DeviceName::Guiderport;

DeviceName	GuiderPort::defaultname(const DeviceName& parent,
			const std::string& unitname) {
	return DeviceName(parent, DeviceName::Guiderport, unitname);
}

GuiderPort::GuiderPort(const DeviceName& name)
	: Device(name, DeviceName::Guiderport) {
}

GuiderPort::GuiderPort(const std::string& name)
	: Device(name, DeviceName::Guiderport) {
}

GuiderPort::~GuiderPort() {
}

} // namespace camera
} // namespace astro