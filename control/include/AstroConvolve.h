/*
 * AstroConvolve.h -- methods and classes to convolve images e.g. with
 *                    point spread functions
 *
 * (c) 2015 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef _AstroConvolve_h
#define _AstroConvolve_h

#include <AstroImage.h>
#include <AstroTypes.h>
#include <fftw3.h>

namespace astro {
namespace image {

class FourierImage;
typedef std::shared_ptr<FourierImage>	FourierImagePtr;

/**
 * \brief Fourier Image
 *
 * This class contains the fourier transform of an image
 */
class FourierImage : public Image<double> {
	ImageSize	_orig;
public:
	const ImageSize&	orig() const { return _orig; }
private:
	void	fourier(const Image<double>& image);
public:
static ImageSize	fsize(const ImageSize&);
	FourierImage(const ImageSize& size);
	FourierImage(const Image<double>& image);
	FourierImage(const ConstImageAdapter<double>& image);
	FourierImage(const ImagePtr image);
	ImagePtr	inverse() const;
	ImagePtr	abs() const;
	ImagePtr	phase() const;
	ImagePtr	color() const;
};

/**
 * \brief multiply two fourier transforms
 */
FourierImagePtr	operator*(const FourierImage& a, const FourierImage& b);
FourierImagePtr	operator*(const FourierImagePtr a, const FourierImagePtr b);
FourierImagePtr	operator/(const FourierImage& a, const FourierImage& b);
FourierImagePtr	operator/(const FourierImagePtr a, const FourierImagePtr b);

class ConvolutionResult;
typedef std::shared_ptr<ConvolutionResult>	ConvolutionResultPtr;

/**
 * \brief class to encode convolution results
 */
class ConvolutionResult : public FourierImage {
	Point	_center;
public:
	const Point	center() const { return _center; }
	void	center(const Point& c) { _center = c; }
public:
	ConvolutionResult(const ImageSize& size, const Point& center);
	ConvolutionResult(const Image<double>& image, const Point& center);
	ConvolutionResult(const ConstImageAdapter<double>& image,
		const Point& center);
	ConvolutionResult(const ImagePtr image, const Point& center);
	ImagePtr	image() const;
};

ConvolutionResultPtr	operator*(const ConvolutionResult& a,
				const ConvolutionResult& b);
ConvolutionResultPtr	operator*(const ConvolutionResultPtr a,
				const ConvolutionResultPtr b);
ConvolutionResultPtr	operator*(const ConvolutionResultPtr a,
				const ConvolutionResult b);
ConvolutionResultPtr	operator*(const ConvolutionResult a,
				const ConvolutionResultPtr b);

/**
 * \brief Base class for convolution operators
 */
class ConvolutionOperator {
	Point	_center;
	ConvolutionResultPtr	_psf;
public:
	const Point	center() const { return _center; }
	void	center(const Point& c) { _center = c; }
public:
	FourierImagePtr	operator()(ImagePtr image) const;
	FourierImagePtr	operator()(FourierImagePtr fourier) const;
};

/**
 * \brief Deconvolution algorithms all use the same base class
 */
class DeconvolutionOperator {
public:
	virtual ImagePtr	operator()(ImagePtr image) const;
	virtual ImagePtr	operator()(FourierImagePtr image) const;
};

/**
 * \brief Basic deconvolution algorithm
 *
 * This algorithm works for any point spread function, but it requires lots
 * of Fourier transforms and thus is rather expensive.
 */
class BasicDeconvolutionOperator {
	FourierImagePtr	_psf;
private:
	BasicDeconvolutionOperator(const BasicDeconvolutionOperator& other);
	BasicDeconvolutionOperator&	operator=(const BasicDeconvolutionOperator& other);
public:
	BasicDeconvolutionOperator(ImagePtr psf);
	BasicDeconvolutionOperator(const ConstImageAdapter<double>& image);
	virtual ImagePtr	operator()(ImagePtr image) const;
};

ImagePtr	smallConvolve(const ConstImageAdapter<double>& small, ImagePtr image);

class VanCittertOperator {
	Image<double>	_psf;
	int	_iterations;
public:
	int	iterations() const { return _iterations; }
	void	iterations(int i) { _iterations = i; }
	VanCittertOperator(ImagePtr psf);
	virtual ImagePtr	operator()(ImagePtr image) const;
};

/**
 * \brief Base class for rotationally symmetric images
 */
class CircularImage : public ConstImageAdapter<double> {
private:
	ImagePoint	_center;
public:
	const ImagePoint&	center() const { return _center; }
private:
	double	_angularpixelsize;
public:
	double	angularpixelsize() const { return _angularpixelsize; }
private:
	double	_weight;
public:
	double	weight() const { return _weight; }
	void	weight(double w) { _weight = w; }
	double	r(int x, int y) const;
	CircularImage(const ImageSize& size, const ImagePoint& center,
		double angularpixelsize, double weight);
	virtual double	totalweight() const;
};

/**
 * \brief Airy disk image
 */
class AiryImage : public CircularImage {
	double	_a;
	double	_k;
public:
	AiryImage(const ImageSize& size, const ImagePoint& center, double a,
		double angularpixelsize,
		double lambda = 550e-9 /* wavelength in meters */);
	virtual double	pixel(int x, int y) const;
};

/**
 * \brief Gauss disk image
 */
class GaussImage : public CircularImage {
	double	_sigma;	// standard deviation
	double	_n;	// normalization constant
public:
	GaussImage(const ImageSize& size, const ImagePoint& center,
		double sigma, double angularpixelsize,
		double weight = 1.0);
	virtual double	pixel(int x, int y) const;
};

class TiledGaussImage : public GaussImage {
	double	_sigma;	// standard deviation
	double	_n;	// normalization constant
	int	_w;
	int	_h;
public:
	TiledGaussImage(const ImageSize& size, double sigma,
		double angularpixelsize, double weight = 1.0);
	virtual double	pixel(int x, int y) const;
};
 
/**
 * \brief circular disk image
 */
class DiskImage : public CircularImage {
	double	_r;
	double	_interiorvalue;
public:
	DiskImage(const ImageSize& size, const ImagePoint& center,
		double r, double angularpixelsize, double weight = 1.0);
	virtual double	pixel(int x, int y) const;
	virtual double	totalweight() const;
};

/**
 * \brief ring image
 */
class RingImage : public CircularImage {
	double	_r_inner;
	double	_r_outer;
	double	_interiorvalue;
public:
	RingImage(const ImageSize& size, const ImagePoint& center,
		double r_inner, double r_outer, double angularpixelsize,
		double weight = 1.0);
	virtual double	pixel(int x, int y) const;
	virtual double	totalweight() const;
};

/**
 * \brief amplifier glow image
 */
class AmplifierGlowImage : public CircularImage {
	double	_r;
public:
	AmplifierGlowImage(const ImageSize& size, const ImagePoint& center,
		double angularpixelsize, double weight, double r);
	virtual double	pixel(int x, int y) const;
};

} // namespace image
} // namespace astro

#endif /* _AstroConvolve_h */
