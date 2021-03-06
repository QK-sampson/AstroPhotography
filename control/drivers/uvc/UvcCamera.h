/*
 * UvcCamera.h -- USB Video Class camera
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef _UvcCamera_h
#define _UvcCamera_h

#include <AstroUVC.h>
#include <AstroCamera.h>

using namespace astro::usb;
using astro::usb::uvc::FrameDescriptor;
using astro::usb::uvc::FormatDescriptor;
using astro::usb::uvc::HeaderDescriptor;

namespace astro {
namespace camera {
namespace uvc {

class UvcCamera : public Camera {
	DevicePtr	deviceptr;
	astro::usb::uvc::UVCCamera	camera;
	typedef struct uvcccd_s {
		int	interface;
		int	format;
		int	frame;
		std::string	guid;
	} uvcccd_t;
	std::vector<uvcccd_t>	ccds;

private:
	// auxiliary functions needed to build the CCD list
	void    addFrame(int interface, int format, int frame,
			const std::string& guid,
			FrameDescriptor *framedescriptor);
	void    addFormat(int interface, int format, 
			FormatDescriptor *formatdescriptor);
	void    addHeader(int interface,
			HeaderDescriptor *headerdescriptor);
public:
	UvcCamera(DevicePtr& deviceptr);
	virtual ~UvcCamera();
protected:
	virtual CcdPtr	getCcd0(size_t ccdindex);

public:
	void	selectFormatAndFrame(int interface, int format, int frame);
	void	setExposureTime(double exposuretime);

	// gain related methods
	bool	hasGain();
	void	setGain(double gain);
	std::pair<float, float>	getGainInterval();

	void	disableAutoWhiteBalance();
	std::vector<FramePtr>	getFrames(int interface, unsigned int nframes);
};

} // namespace uvc
} // namespace camera
} // namespace astro

#endif /* _UvcCamera_h */
