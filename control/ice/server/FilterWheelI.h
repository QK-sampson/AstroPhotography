/*
 * FilterWheelI.h -- filter wheel implementation
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef _FilterWheelI_h
#define _FilterWheelI_h

#include <camera.h>
#include <AstroCamera.h>
#include <DeviceI.h>

namespace snowstar {

class FilterWheelI : virtual public FilterWheel, virtual public DeviceI {
	astro::camera::FilterWheelPtr	_filterwheel;
public:
	FilterWheelI(astro::camera::FilterWheelPtr filterwheel);
	virtual	~FilterWheelI();
	virtual int	nFilters(const Ice::Current& current);
	virtual int	currentPosition(const Ice::Current& current);
	virtual void	select(int, const Ice::Current& current);
	virtual void	selectName(const std::string& filtername,
		const Ice::Current& current);
	virtual std::string	filterName(int, const Ice::Current& current);
	virtual FilterwheelState	getState(const Ice::Current& current);
static	FilterWheelPrx	createProxy(const std::string& filterwheelname,
		const Ice::Current& current);
};

} // namespace snowstar

#endif /* _FilterWheelI_h */
