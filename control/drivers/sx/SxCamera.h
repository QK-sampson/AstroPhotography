/*
 * SxCamera.h -- starlight express camera declarations
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef _SxCamera_h
#define _SxCamera_h

#include <AstroCamera.h>
#include <AstroUSB.h>
#include <sx.h>

using namespace astro::camera;
using namespace astro::usb;

namespace astro {
namespace camera {
namespace sx {

class SxCamera : public Camera {
	DevicePtr	deviceptr;
	uint16_t	model;
	uint16_t	product;
	InterfacePtr	interface;
	EndpointDescriptorPtr	outendpoint;
	EndpointDescriptorPtr	inendpoint;
	bool	useControlRequests;
	sx_firmware_version_t	firmware_version;
	bool	hasCooler;
	bool	hasGuiderPort;
public:
	// USB related methods
	DevicePtr	getDevicePtr();
	EndpointDescriptorPtr	getEndpoint();
	InterfacePtr	getInterface();

	// constructors
	SxCamera(DevicePtr& devptr);
	virtual ~SxCamera();

	// ccd access
	virtual CcdPtr	getCcd(size_t id);

	// cooler access
	virtual CoolerPtr	getCooler(int ccdindex);

	// guider port access
	virtual GuiderPortPtr	getGuiderPort() throw (not_implemented);

	// request handling
	void	controlRequest(RequestBase *request);
};


} // namespace sx
} // namespace camera
} // namespace astro

#endif /* _SxCamera_h */
