/*
 * Mock1Ccd.h -- mock ccd implementation
 *
 * (c) 2012 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef _Mock1Ccd_h
#define _Mock1Ccd_h

#include <AstroCamera.h>

using namespace astro::image;
using namespace astro::camera;

namespace astro {
namespace camera {
namespace mock1 {

class Mock1Ccd : public Ccd {
	int	cameraid;
	int	ccd;
	ImageRectangle	frame;
public:
	Mock1Ccd(const CcdInfo& info, int _cameraid, int _ccd)
		: Ccd(info), cameraid(_cameraid), ccd(_ccd) { }
	virtual ~Mock1Ccd() { }
	virtual void    startExposure(const Exposure& exposure);
	virtual CcdState::State exposureStatus();
	virtual void    cancelExposure();
	virtual ImagePtr    getRawImage();
}; 

} // namespace mock1
} // namespace camera
} // namespace astro

#endif /* _Mack1Ccd_h */
