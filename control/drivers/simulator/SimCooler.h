/*
 * SimCooler.h -- Cooler simulator
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef _SimCooler_h
#define _SimCooler_h

#include <SimLocator.h>

namespace astro {
namespace camera {
namespace simulator {

class SimCooler : public Cooler {
	SimLocator&	_locator;
	double	laststatechange;
	float	lasttemperature;
	bool	on;
public:
	SimCooler(SimLocator& locator);
	virtual float	getActualTemperature();
	virtual void	setTemperature(float _temperature);
	virtual void	setOn(bool onoff);
	virtual bool	isOn() { return on; }
	int	belowambient();
};

} // namespace simulator
} // namespace camera
} // namespace astro

#endif /* _SimCooler_h */
