/*
 * AstroChart.h -- Using Star catalogs to create charts
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef _AstroChart_h
#define _AstroChart_h

#include <AstroCoordinates.h>
#include <AstroCatalog.h>
#include <AstroProjection.h>
#include <AstroConvolve.h>

namespace astro {
namespace catalog {

/**
 * \brief ImageGeometry
 */
class ImageGeometry : public ImageSize {
	double	_pixelsize;
public:
	double	pixelsize() const { return _pixelsize; }
	void	pixelsize(double p) { _pixelsize = p; }
private:
	double	_focallength;
public:
	double	focallength() const { return _focallength; }
	void	focallength(double f) { _focallength = f; }
private:
	double	_aperture;
public:
	double	aperture() const { return _aperture; }
	void	aperture(double o) { _aperture = o; }
public:
	ImageGeometry();
	ImageGeometry(const ImageSize& size, double focallength,
		double pixelsize);
	ImageGeometry(const ImageBase& image);
	void	addMetadata(ImageBase& image) const;
	Angle	rawidth() const;
	Angle	decheight() const;
	virtual std::string	toString() const;
	Point	coordinates(const Point& a) const;
	double	angularpixelsize() const;
	double	resolutionangle() const;
	double	f() const { return _aperture / _focallength; }
};

/**
 * \brief A rectangle on the sky
 */
class SkyRectangle : public SkyWindow {
	UnitVector	direction;
	UnitVector	rightvector;
	UnitVector	upvector;
	double	uplimit;
	double	rightlimit;
	void	setup();
public:
	SkyRectangle();
	SkyRectangle(const SkyWindow& window);
	SkyRectangle(const ImageBase& image);
	SkyRectangle(const RaDec& center, const ImageGeometry& geometry);
	bool	contains(const RaDec& point) const;
	astro::Point	map(const RaDec& where) const;
	astro::Point	map2(const RaDec& where) const;
	astro::Point	point(const ImageSize& size, const RaDec& where) const;
	SkyWindow	containedin() const;
	RaDec	inverse(const astro::Point& p) const;
	void	addMetadata(ImageBase& image) const;
};

class ChartFactory;
/**
 * \brief Chart abstraction
 *
 * A Chart consists of an image and SkyRectangle that defines the
 * coordinate system.
 */
class Chart {
private:
	SkyRectangle	_rectangle;
	ImageSize	_size;
	Image<double>	*_image;
	ImagePtr	_imageptr;
public:
	Chart(const SkyRectangle rectangle, const ImageSize& size);
	const SkyRectangle&	rectangle() const { return _rectangle; }
	const ImageSize&	size() const { return _size; }
	const ImagePtr	image() const { return _imageptr; }
friend class ChartFactory;
};

/**
 * \brief Point spread functions
 */
class PointSpreadFunction {
public:
	virtual double	operator()(double r) const = 0;
};

/**
 * \brief Dirac Point spread function
 */
class DiracPointSpreadFunction : public PointSpreadFunction {
public:
	DiracPointSpreadFunction() { }
	/**
	 * \brief Point spread function
	 *
	 * The point spread function is the probability distribution
	 * of the light depending on the angular distance from the point
	 */
	virtual double operator()(double r) const;
};

/**
 * \brief Point spread function the just turns a star into a circle
 */
class CirclePointSpreadFunction : public PointSpreadFunction {
	double	_maxradius;
public:
	CirclePointSpreadFunction(double maxradius) : _maxradius(maxradius) { }
	virtual double	operator()(double r) const;
};

/**
 * \brief The point spread function for an image dominated by diffraction
 */
class DiffractionPointSpreadFunction : public PointSpreadFunction {
	double	_xfactor;
public:
	DiffractionPointSpreadFunction(const ImageGeometry& geometry);
	virtual double	operator()(double r) const;
};

/**
 * \brief The point spread function for an image dominated by turbulence
 */
class TurbulencePointSpreadFunction : public PointSpreadFunction {
	double	_norm;
	double	_turbulence;
public:
	TurbulencePointSpreadFunction(double turbulence = 2.);
	virtual double	operator()(double r) const;
};

class PointSpreadFunctionAdapter : public CircularImage {
	const PointSpreadFunction&	_pointspreadfunction;
public:
	PointSpreadFunctionAdapter(const ImageSize& size,
		const ImagePoint& center, double angularpixelsize,
		const PointSpreadFunction& pointspreadfunction);
	virtual double	pixel(int x, int y) const;
};

class ChartFactoryBase {
// parameters valid for all images
protected:
	CatalogPtr	_catalog;
	const PointSpreadFunction&	pointspreadfunction;
private:
	double	_limit_magnitude;
public:
	double	limit_magnitude() const { return _limit_magnitude; }
	void	limit_magnitude(double l) { _limit_magnitude = l; }
private:
	double	_scale;
public:
	double	scale() const { return _scale; }
	void	scale(double s) { _scale = s; }
private:
	bool	_logarithmic;
public:
	bool	logarithmic() const { return _logarithmic; }
	void	logarithmic(bool l) { _logarithmic = l; }
public:
	// constructors
	ChartFactoryBase(CatalogPtr catalog, const PointSpreadFunction& psf,
		double limit_magnitude = 16, double scale = 1,
		bool logarithmic = false)
		: _catalog(catalog), pointspreadfunction(psf),
		  _limit_magnitude(limit_magnitude),
		  _scale(scale),
		  _logarithmic(logarithmic) {
	}

protected:
	void	draw(Image<double>& image, const Point& p,
			const Star& star) const;
	void	limit(Image<double>& image, double limit) const;
	void	spread(Image<double>& image, int morepixels,
			const ImageGeometry& geometry) const;
};

/**
 * \brief Chart factory abstraction
 *
 * Class to produce charts for sets of stars. 
 */
class ChartFactory : public ChartFactoryBase {
public:
	// constructors
	ChartFactory(CatalogPtr catalog, const PointSpreadFunction& psf,
		double limit_magnitude = 16, double scale = 1,
		bool logarithmic = false)
		: ChartFactoryBase(catalog, psf, limit_magnitude, scale,
		  logarithmic) {
	}

	// functions needed to produce a chart
	Chart	chart(const RaDec& center, const ImageGeometry& geometry) const;
private:
	void	draw(Image<double>& image, const SkyRectangle& rectangle,
			const Catalog::starset& star) const;
	void	draw(Image<double>& image, const SkyRectangle& rectangle,
			const Catalog::starsetptr star) const;
	void	draw(Image<double>& image, const SkyRectangle& rectangle,
			const Star& star) const;
};

/**
 * \brief Normalize an image
 *
 * Find a projection 
 */
class ImageNormalizer {
	ChartFactory&	_factory;
public:
	ImageNormalizer(ChartFactory& factory);
	RaDec	operator()(astro::image::ImagePtr image,
			astro::image::transform::Projection& projection);
};

/**
 * \brief StereographicChart
 */
class StereographicChart {
	RaDec	_center;
	Image<double>	*_image;
	ImagePtr	_imageptr;
public:
	StereographicChart(const RaDec& center, unsigned int diameter);
	const ImageSize&	size() const { return _imageptr->size(); }
	const ImagePtr	image() const { return _imageptr; }
friend class StereographicChartFactory;
};

/**
 * \brief Factory for stereographic charts
 */
class StereographicChartFactory : public ChartFactoryBase {
public:
	StereographicChartFactory(CatalogPtr catalog,
		const PointSpreadFunction& psf, double limit_magnitude = 16,
		double scale = 1,
		bool logarithmic = false)
		: ChartFactoryBase(catalog, psf, limit_magnitude, scale,
		  logarithmic) {
	}

	StereographicChart	chart(const RaDec& center,
					unsigned int diameter) const;
private:
	void	draw(Image<double>& image,
			const astro::image::transform::StereographicProjection& projection,
			const Star& star) const;

	void	draw(Image<double>& image,
			const astro::image::transform::StereographicProjection& projection,
			const Catalog::starsetptr stars) const;

	void	draw(Image<double>& image,
			const astro::image::transform::StereographicProjection& projection,
			const Catalog::starset& stars) const;
};

} // namespace catalog
} // namespace astro

#endif /* _AstroChart_h */
