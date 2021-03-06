/*
 * Starfield.cpp -- starfield implementation
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <Stars.h>

using namespace astro::image;

namespace astro {

/**
 * \brief Create a new star field
 *
 * \param size		The image field size
 * \param overshoot	how many pixels to add on each side of the frame
 * \param nobjects	number of stars to generate
 */
StarField::StarField(const ImageSize& size, int overshoot,
	unsigned int nobjects) {
	for (unsigned int i = 0; i < nobjects; i++) {
		createStar(size, overshoot);
	}
}

/**
 * \brief Create a random star
 *
 * Stars are evenly distributed in the rectangle formed by adding
 * overshoot to the camera frame on each side. The magnitudes follow
 * a power distribution, which may not be entirely accurate, but is
 * a sufficiently good model for this simulation.
 */
void	StarField::createStar(const ImageSize& size, int overshoot) {
	int	x = (random() % (size.width() + 2 * overshoot)) - overshoot;
	int	y = (random() % (size.height() + 2 * overshoot)) - overshoot;
	// create magnitudes with a power distribution
	double	magnitude = log2(8 + (random() % 56)) - 3;

	StellarObject	*newstar = new Star(Point(x, y), magnitude);

	// create color
	int	colorcode = random() % 8;
	double	red = (colorcode & 4) ? 1.0 : 0.8;
	double	green = (colorcode & 2) ? 1.0 : 0.8;
	double	blue = (colorcode & 1) ? 1.0 : 0.8;
	RGB<double>	color(red, green, blue);
	newstar->color(color);
	//debug(LOG_DEBUG, DEBUG_LOG, 0, "color: %.2f/%.2f/%.2f",
	//	red, green, blue);
	
	// the new star
	addObject(StellarObjectPtr(newstar));
}

/**
 * \brief Add a new stellar object
 *
 * This method accepts stars or nebulae
 */
void	StarField::addObject(StellarObjectPtr object) {
	objects.push_back(object);
}

/**
 * \brief Compute cumulated intensity for all objects in the star field
 */
double	StarField::intensity(const Point& where) const {
	std::vector<StellarObjectPtr>::const_iterator	i;
	double	result = 0;
	for (i = objects.begin(); i != objects.end(); i++) {
		result += (*i)->intensity(where);
	}
	return result;
}

/**
 * \brief Compute cumulated intensity for all objects in the star field
 */
double	StarField::intensityR(const Point& where) const {
	std::vector<StellarObjectPtr>::const_iterator	i;
	double	result = 0;
	for (i = objects.begin(); i != objects.end(); i++) {
		result += (*i)->intensityR(where);
	}
	return result;
}

/**
 * \brief Compute cumulated intensity for all objects in the star field
 */
double	StarField::intensityG(const Point& where) const {
	std::vector<StellarObjectPtr>::const_iterator	i;
	double	result = 0;
	for (i = objects.begin(); i != objects.end(); i++) {
		result += (*i)->intensityG(where);
	}
	return result;
}

/**
 * \brief Compute cumulated intensity for all objects in the star field
 */
double	StarField::intensityB(const Point& where) const {
	std::vector<StellarObjectPtr>::const_iterator	i;
	double	result = 0;
	for (i = objects.begin(); i != objects.end(); i++) {
		result += (*i)->intensityB(where);
	}
	return result;
}

} // namespace astro
