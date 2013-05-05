/*
 * UVCCamera.cpp -- UVC Camera implementation
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <UVCCamera.h>
#include <debug.h>
#include <Format.h>
#include <UVCCcd.h>
#include <UVCUtils.h>

namespace astro {
namespace camera {
namespace uvc {

using astro::usb::uvc::HeaderDescriptor;

void	UVCCamera::addFrame(int interface, int format, int frame,
	FrameDescriptor *framedescriptor) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "interface %d, format %d, frame %d",
		interface, format, frame);

	// UVC interface/format/frame information
	uvcccd_t	uvcccd;
	uvcccd.interface = interface;
	uvcccd.format = format;
	uvcccd.frame = frame;
	ccds.push_back(uvcccd);

	// standard CcdInfo
	CcdInfo	ccd;
	ccd.size = astro::image::ImageSize(framedescriptor->wWidth(),
		framedescriptor->wHeight());
	ccd.name = stringprintf("%dx%d/%d/%d/%d",
		ccd.size.width, ccd.size.height,
		uvcccd.interface, uvcccd.format, uvcccd.frame);
	ccd.binningmodes.push_back(Binning(1,1));
	ccdinfo.push_back(ccd);

	// add ccdinfo
	debug(LOG_DEBUG, DEBUG_LOG, 0, "adding CCD %s", ccd.getName().c_str());
}

void	UVCCamera::addFormat(int interface, int format,
		FormatDescriptor *formatdescriptor) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "interface %d, format %d",
		interface, format);
	// descriptor type must be an interface specific descriptor
	int	type = formatdescriptor->bDescriptorType();
	if (type != CS_INTERFACE) {
		return;
	}
	// subtype must be uncompressed or frame based
	int	subtype = formatdescriptor->bDescriptorSubtype();
	switch (subtype) {
	case VS_FORMAT_UNCOMPRESSED:
	case VS_FORMAT_FRAME_BASED:
		break;
	default:
		return;
	}
	// if we get to this point, then we can add all the frames
	int	framecount = formatdescriptor->numFrames();
	debug(LOG_DEBUG, DEBUG_LOG, 0, "frames: %d", framecount);
	for (int frameindex = 1; frameindex <= framecount; frameindex++) {
		FrameDescriptor	*framedescriptor
			= getPtr<FrameDescriptor>((*formatdescriptor)[frameindex - 1]);
		addFrame(interface, format, frameindex, framedescriptor);
	}
}

void	UVCCamera::addHeader(int interface, HeaderDescriptor *headerdescriptor) {
	int	formatcount = headerdescriptor->bNumFormats();
	debug(LOG_DEBUG, DEBUG_LOG, 0, "%d formats", formatcount);

	for (int findex = 1; findex <= formatcount; findex++) {
		USBDescriptorPtr	formatptr
			= camera.getFormatDescriptor(interface, findex);
		FormatDescriptor	*formatdescriptor
			= getPtr<FormatDescriptor>(formatptr);
		addFormat(interface, findex, formatdescriptor);
	}
}

UVCCamera::UVCCamera(DevicePtr& _deviceptr) : deviceptr(_deviceptr),
	camera(*deviceptr, true) {
	// show what we have in this camera
	std::cout << camera;

	// find out how many different formats this camera has, we are
	// only interested in frames that are uncompressed or frame based,
	// all other types are not acceptable
	int	firstinterface = camera.iad().bFirstInterface();
	int	interfacecount = camera.iad().bInterfaceCount();
	debug(LOG_DEBUG, DEBUG_LOG, 0, "streaming interfaces: %d",
		interfacecount - 1);
	int	lastinterface = firstinterface + interfacecount;
	for (int ifno = firstinterface + 1; ifno < lastinterface; ifno++) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "interface %d", ifno);
		USBDescriptorPtr	header
			= camera.getHeaderDescriptor(ifno);
		// find out how many formats this header contains
		HeaderDescriptor	*hd = getPtr<HeaderDescriptor>(header);
		addHeader(ifno, hd);
	}
}

UVCCamera::~UVCCamera() {
}

CcdPtr	UVCCamera::getCcd(const std::string& name) {
	// locate the camera using the name
	return CcdPtr();
}

CcdPtr	UVCCamera::getCcd(int ccdindex) {
	uvcccd_t	uc = ccds[ccdindex];
	CcdInfo	info = ccdinfo[ccdindex];
	return CcdPtr(new UVCCcd(info, uc.interface, uc.format, uc.frame,
		*this));
}

void	UVCCamera::selectFormatAndFrame(int interface, int format, int frame) {
	try {
		camera.selectFormatAndFrame(interface, format, frame);
	} catch (std::exception& x) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "cannot select interface %d, "
			"format %d, frame %d: %s",
			interface, format, frame, x.what());
		throw UVCError("cannot set format/frame");
	}
}

void	UVCCamera::setExposureTime(double exposuretime) {
	try {
		camera.setExposureTime(exposuretime);
	} catch (std::exception& x) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "cannot set exposure time: %s",
			x.what());
		throw UVCError("cannot set exposure time");
	}
}

void	UVCCamera::setGain(double gain) {
	try {
		camera.setGain(gain);
	} catch (std::exception& x) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "cannot set gain: %s",
			x.what());
		throw UVCError("cannot set exposure time");
	}
}

} // namespace uvc
} // namespace camera
} // namespace astro