/*
 * AstroAdapter.h -- a collection of adapters
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef _AstroAdapter_h
#define _AstroAdapter_h

#include <AstroImage.h>
#include <AstroMask.h>
#include <AstroDebug.h>
#include <AstroTypes.h>

using namespace astro::image;

namespace astro {
namespace adapter {

//////////////////////////////////////////////////////////////////////
// Identity adapter
//////////////////////////////////////////////////////////////////////
template<typename Pixel>
class IdentityAdapter : public ConstImageAdapter<Pixel> {
	const ConstImageAdapter<Pixel>&	_image;
public:
	IdentityAdapter(const ConstImageAdapter<Pixel>& image)
		: ConstImageAdapter<Pixel>(image.getSize()), _image(image) {
	}
	virtual Pixel	pixel(unsigned int x, unsigned int y) const {
		return _image.pixel(x, y);
	}
};

//////////////////////////////////////////////////////////////////////
// Accessing Subrectangles of an Image
//////////////////////////////////////////////////////////////////////

/**
 * \brief Adapter for a subimage
 *
 * This adapter allows to treat a subrectangle of an image just as if it
 * were the image itself, except that the image cannot be changed.
 */
template<typename Pixel>
class WindowAdapter : public ConstImageAdapter<Pixel> {
	const ConstImageAdapter<Pixel>&	image;
	ImageRectangle	frame;
public:
	WindowAdapter(const ConstImageAdapter<Pixel>& image, const ImageRectangle& frame);
	
	virtual Pixel	pixel(unsigned int x, unsigned int y) const;
};

/**
 * \brief Construct a subimage adapter
 */
template<typename Pixel>
WindowAdapter<Pixel>::WindowAdapter(const ConstImageAdapter<Pixel>& _image,
	const ImageRectangle& _frame)
	: ConstImageAdapter<Pixel>(_frame.size()),
	  image(_image), frame(_frame) {
}

/**
 * \bief Access pixel inside the subwindow
 */
template<typename Pixel>
Pixel	WindowAdapter<Pixel>::pixel(unsigned int x, unsigned int y) const {
	return	image.pixel(frame.origin().x() + x, frame.origin().y() + y);
}

template<typename Pixel>
class SubimageAdapter : public ImageAdapter<Pixel> {
	ImageAdapter<Pixel>&	_image;
	ImageRectangle	_frame;
public:
	SubimageAdapter(ImageAdapter<Pixel>& image, const ImageRectangle& frame)
		: ImageAdapter<Pixel>(frame.size()), _image(image),
		  _frame(frame) {
		if (!ImageRectangle(image.getSize()).contains(frame)) {
			throw std::runtime_error("frame not inside image");
		}
	}
	virtual Pixel& writablepixel(unsigned int x, unsigned int y) {
		ImagePoint	p = _frame.subimage(x, y);
		return _image.writablepixel(p.x(), p.y());
	}
	virtual Pixel	pixel(unsigned int x, unsigned int y) const {
		ImagePoint	p = _frame.subimage(x, y);
		return _image.pixel(p.x(), p.y());
	}
};

//////////////////////////////////////////////////////////////////////
// Copying image
//////////////////////////////////////////////////////////////////////
template<typename dstPixel, typename srcPixel>
void	copy(ImageAdapter<dstPixel>& target,
		const ConstImageAdapter<srcPixel>& source) {
	if (target.getSize() != source.getSize()) {
		throw std::runtime_error("image copy size mismatch");
	}
	unsigned int	w = source.getSize().width();
	unsigned int	h = source.getSize().height();
	for (unsigned int x = 0; x < w; x++) {
		for (unsigned int y = 0; y < h; y++) {
			target.writablepixel(x, y) = source.pixel(x, y);
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Converting Pixel values
//////////////////////////////////////////////////////////////////////
/**
 * \brief Adapter to subimage with implied pixel type conversion
 */
template<typename TargetPixel, typename SourcePixel>
class ConvertingAdapter : public ConstImageAdapter<TargetPixel> {
	const ConstImageAdapter<SourcePixel>&	 image;
public:
	ConvertingAdapter(const ConstImageAdapter<SourcePixel>& image);
	virtual TargetPixel	pixel(unsigned int x, unsigned int y) const;
};

template<typename TargetPixel, typename SourcePixel>
ConvertingAdapter<TargetPixel, SourcePixel>::ConvertingAdapter(
	const ConstImageAdapter<SourcePixel>& _image)
	: ConstImageAdapter<TargetPixel>(_image.getSize()), image(_image) {
}

template<typename TargetPixel, typename SourcePixel>
TargetPixel	ConvertingAdapter<TargetPixel, SourcePixel>::pixel(unsigned int x, unsigned int y) const {
	const SourcePixel	t = image.pixel(x, y);
	// convert to Pixel type
	TargetPixel	p(t);
	return p;
}

//////////////////////////////////////////////////////////////////////
// Adapter for access to a subgrid
//////////////////////////////////////////////////////////////////////

/**
 * \brief Adapter to a subgrid
 */
template<typename Pixel>
class ConstSubgridAdapter : public ConstImageAdapter<Pixel> {
	const ConstImageAdapter<Pixel>&	image;
	Subgrid	subgrid;
public:
	ConstSubgridAdapter(const ConstImageAdapter<Pixel>& image,
		const Subgrid& subgrid);
	virtual Pixel	pixel(unsigned int x, unsigned int y) const;
};

template<typename Pixel>
ConstSubgridAdapter<Pixel>::ConstSubgridAdapter(
	const ConstImageAdapter<Pixel>& _image, const Subgrid& _subgrid)
	: ConstImageAdapter<Pixel>(ImageSize(
		(_image.getSize().width() - _subgrid.origin.x())
			/ _subgrid.stepsize.width(),
		(_image.getSize().height() - _subgrid.origin.y())
			/ _subgrid.stepsize.height())
	), image(_image), subgrid(_subgrid) {
}
	
template<typename Pixel>
Pixel	ConstSubgridAdapter<Pixel>::pixel(unsigned int x,
			unsigned int y) const {
	return image.pixel(subgrid.x(x), subgrid.y(y));
}

/**
 * \brief Mutable adapter to a subgrid
 */
template<typename Pixel>
class SubgridAdapter : public ImageAdapter<Pixel> {
	ImageAdapter<Pixel>&	image;
	Subgrid	subgrid;
public:
	SubgridAdapter(ImageAdapter<Pixel>& image,
		const Subgrid& subgrid);
	virtual Pixel	pixel(unsigned int x, unsigned int y) const;
	virtual Pixel&	writablepixel(unsigned int x, unsigned int y);
};

template<typename Pixel>
SubgridAdapter<Pixel>::SubgridAdapter(
	ImageAdapter<Pixel>& _image, const Subgrid& _subgrid)
	: ImageAdapter<Pixel>(ImageSize(
		(_image.getSize().width() - _subgrid.origin.x())
			/ _subgrid.stepsize.width(),
		(_image.getSize().height() - _subgrid.origin.y())
			/ _subgrid.stepsize.height())
	), image(_image), subgrid(_subgrid) {
}
	
template<typename Pixel>
Pixel	SubgridAdapter<Pixel>::pixel(unsigned int x,
			unsigned int y) const {
	return image.pixel(subgrid.x(x), subgrid.y(y));
}

template<typename Pixel>
Pixel&	SubgridAdapter<Pixel>::writablepixel(unsigned int x, unsigned int y) {
	return image.writablepixel(subgrid.x(x), subgrid.y(y));
}

//////////////////////////////////////////////////////////////////////
// Adapter for arithmetic operations
//////////////////////////////////////////////////////////////////////

/**
 * \brief Base class for arithmetic operation adapters
 */ 
template<typename Pixel>
class ArithmeticAdapter : public ConstImageAdapter<double> {
protected:
	const ConstImageAdapter<Pixel>&	operand1;
	const ConstImageAdapter<Pixel>&	operand2;
public:
	ArithmeticAdapter(const ConstImageAdapter<Pixel>& summand1,
		const ConstImageAdapter<Pixel>& summand2);
};

/**
 * \brief Constructor
 *
 * The constructor verifies that the two operands have the same size
 */
template<typename Pixel>
ArithmeticAdapter<Pixel>::ArithmeticAdapter(
	const ConstImageAdapter<Pixel>& _operand1,
	const ConstImageAdapter<Pixel>& _operand2)
	: ConstImageAdapter<Pixel>(_operand1.getSize()),
	  operand1(_operand1), operand2(_operand2) {
	// verify that the two operands have the same size
	if (operand1.getSize() != operand2.getSize()) {
		throw std::runtime_error("summand size does not match");
	}
}

/**
 * \brief Add adapter
 *
 * Can be used to add two Images
 */
template<typename Pixel>
class AddAdapter : public ArithmeticAdapter<Pixel> {
public:
	AddAdapter(const ConstImageAdapter<Pixel>& summand1,
		const ConstImageAdapter<Pixel>& summand2);
	virtual double	pixel(unsigned int x, unsigned int y) const;
};

/**
 * \brief Construct an addition adapter
 */
template<typename Pixel>
AddAdapter<Pixel>::AddAdapter(const ConstImageAdapter<Pixel>& summand1,
		const ConstImageAdapter<Pixel>& summand2)
	: ArithmeticAdapter<Pixel>(summand1, summand2) {
}

/**
 * \brief Perform the addition
 */
template<typename Pixel>
double	AddAdapter<Pixel>::pixel(unsigned int x, unsigned int y) const {
	double	result = 0;
	result += ArithmeticAdapter<Pixel>::operand1.pixel(x, y);
	result += ArithmeticAdapter<Pixel>::operand2.pixel(x, y);
	return result;
}

template<typename Pixel>
class MultiplyAdapter : public ArithmeticAdapter<Pixel> {
public:
	MultiplyAdapter(const ConstImageAdapter<Pixel>& summand1,
		const ConstImageAdapter<Pixel>& summand2);
	virtual double	pixel(unsigned int x, unsigned int y) const;
};

template<typename Pixel>
MultiplyAdapter<Pixel>::MultiplyAdapter(
		const ConstImageAdapter<Pixel>& operand1,
		const ConstImageAdapter<Pixel>& operand2)
	: ArithmeticAdapter<Pixel>(operand1, operand2) {
}

template<typename Pixel>
double	MultiplyAdapter<Pixel>::pixel(unsigned int x, unsigned int y) const {
	double	result = 1;
	result *= ArithmeticAdapter<Pixel>::operand1.pixel(x, y);
	result *= ArithmeticAdapter<Pixel>::operand2.pixel(x, y);
	return result;
}

//////////////////////////////////////////////////////////////////////
// Adapter to compute the Laplacian of an image
//////////////////////////////////////////////////////////////////////
/**
 * \brief Adapter that computes the image Laplacian
 *
 * The Laplacian is used to compute a figure of merit for the focus of an image.
 * There the value if the laplacian is multiplied with the image value
 * at the same point, and everything is integrated. 
 */
template<typename Pixel>
class LaplacianAdapter : public ConstImageAdapter<double> {
	const ConstImageAdapter<Pixel>& image;
	bool	diagonal;
	double	scale;
public:
	LaplacianAdapter(const ConstImageAdapter<Pixel>& image,
		bool diagonal = false);
	double	pixel(unsigned int x, unsigned int y) const;
};

/**
 * \brief Construct a Laplacian Adapter
 */
template<typename Pixel>
LaplacianAdapter<Pixel>::LaplacianAdapter(
	const ConstImageAdapter<Pixel>& _image, bool _diagonal)
	: ConstImageAdapter<double>(_image.getSize()),
	  image(_image), diagonal(_diagonal) {
	if (diagonal) {
		scale = sqrt(2);
	} else {
		scale = 1;
	}
}

/**
 * \brief compute the Laplacian at a given point
 *
 * Note the special treatement of the points at the boundary. In many cases
 * it will be advantegous to apply a mask to the image so that boundary
 * artefacts do not misrepresent e.g. the focus.
 */
template<typename Pixel>
double	LaplacianAdapter<Pixel>::pixel(unsigned int x, unsigned int y)
			const {
	double	result = 0;
	int	counter = 0;
	if (diagonal) {
		if ((x > 0) && (x < adaptersize.width() - 1) &&
			(y > 0) && (y < adaptersize.height() - 1)) {
			result += image.pixel(x - 1, y - 1);
			result += image.pixel(x + 1, y - 1);
			result += image.pixel(x - 1, y + 1);
			result += image.pixel(x + 1, y + 1);
			counter += 4;
		}
	} else {
		if ((x > 0) && (x < adaptersize.width() - 1)) {
			result += image.pixel(x - 1, y);
			result += image.pixel(x + 1, y);
			counter += 2;
		}
		if ((y > 0) && (y < adaptersize.height() - 1)) {
			result += image.pixel(x, y - 1);
			result += image.pixel(x, y + 1);
			counter += 2;
		}
	}
	if (counter == 0) {
		return 0;
	}
	double	center = image.pixel(x, y);
	result -= counter * center;
	return result / (scale * counter);
}

//////////////////////////////////////////////////////////////////////
// Focus Figure of merit adapter for 
//////////////////////////////////////////////////////////////////////
template<typename Pixel>
class FocusFOMAdapter : public ConstImageAdapter<double> {
	LaplacianAdapter<Pixel>	laplacian;
	ConvertingAdapter<double, Pixel>	converting;
	MultiplyAdapter<double>	multiply;
public:
	FocusFOMAdapter(const ConstImageAdapter<Pixel>& image,
		bool diagonal = false);
	double	pixel(unsigned int x, unsigned int y) const;
};

template<typename Pixel>
FocusFOMAdapter<Pixel>::FocusFOMAdapter(const ConstImageAdapter<Pixel>& _image,
	bool diagonal)
	: ConstImageAdapter<double>(ImageSize(_image.getSize().width() - 2,
		_image.getSize().height() - 2)),
	  laplacian(_image, diagonal),
	  converting(_image),
	  multiply(laplacian, converting) {
}

template<typename Pixel>
double FocusFOMAdapter<Pixel>::pixel(unsigned int x, unsigned int y) const {
	return -multiply.pixel(x + 1, y + 1);
}

//////////////////////////////////////////////////////////////////////
// Adapter that applies a masking function to an image
//////////////////////////////////////////////////////////////////////
template<typename Pixel>
class MaskingAdapter : public ConstImageAdapter<Pixel> {
	const ConstImageAdapter<Pixel>&	image;
	const MaskingFunction&	maskingfunction;
public:
	MaskingAdapter(const ConstImageAdapter<Pixel>& image,
		const MaskingFunction& maskingfunction);
	virtual Pixel	pixel(unsigned int x, unsigned int y) const;
};

template<typename Pixel>
MaskingAdapter<Pixel>::MaskingAdapter(const ConstImageAdapter<Pixel>& _image,
	const MaskingFunction& _maskingfunction)
	: ConstImageAdapter<Pixel>(_image.getSize()),
	  image(_image), maskingfunction(_maskingfunction) {
}

template<typename Pixel>
Pixel	MaskingAdapter<Pixel>::pixel(unsigned int x, unsigned int y) const {
	Pixel	v = image.pixel(x, y);
	double m = maskingfunction(x, y);
	v *= m;
	return v;
}

//////////////////////////////////////////////////////////////////////
// Caching adapter
//////////////////////////////////////////////////////////////////////
template<typename Pixel>
class CachingAdapter : public ConstImageAdapter<Pixel> {
	const ConstImageAdapter<Pixel>&	image;
	bool	*tags;
	Pixel	values[];
public:
	CachingAdapter(const ConstImageAdapter<Pixel>& image);
	~CachingAdapter();
	virtual Pixel	pixel(unsigned int x, unsigned int y) const;
};

template<typename Pixel>
CachingAdapter<Pixel>::CachingAdapter(const ConstImageAdapter<Pixel>& _image)
	: ConstImageAdapter<Pixel>(_image.getSize()), image(_image) {
	size_t	pixels = image.getSize().pixels;
	tags = new bool[pixels];
	for (unsigned int i = 0; i < pixels; i++) {
		tags[i] = false;
	}
	values = new Pixel[pixels];
}

template<typename Pixel>
CachingAdapter<Pixel>::~CachingAdapter() {
	delete[] tags;
	delete[] values;
}

template<typename Pixel>
Pixel	CachingAdapter<Pixel>::pixel(unsigned int x, unsigned int y) const {
	// check if a cached value is available
	unsigned int	offset = ConstImageAdapter<Pixel>::adaptersize.offset(x, y);
	if (tags[offset]) {
		return values[offset];
	}
	tags[offset] = true;
	values[offset] = image.pixel(x, y);
	return values[offset];
}

//////////////////////////////////////////////////////////////////////
// Up/Downsampling adapters
//////////////////////////////////////////////////////////////////////
template<typename Pixel>
class DownSamplingAdapter : public ConstImageAdapter<Pixel> {
	const ConstImageAdapter<Pixel>&	image;
	ImageSize	sampling;
	double	*weights;
	unsigned int	volume;
	Pixel	*pixels;
public:
	DownSamplingAdapter(const ConstImageAdapter<Pixel>& image,
		const ImageSize& sampling);
	virtual	~DownSamplingAdapter();
	Pixel	pixel(unsigned int x, unsigned int y) const;
};

template<typename Pixel>
DownSamplingAdapter<Pixel>::DownSamplingAdapter(
	const ConstImageAdapter<Pixel>& _image, const ImageSize& _sampling)
	: ConstImageAdapter<Pixel>(
		ImageSize(_image.getSize().width() / _sampling.width(),
			_image.getSize().height() / _sampling.height())),
	  image(_image), sampling(_sampling) {
	volume = sampling.width() * sampling.height();
	weights = new double[volume];
	weights[0] = 1./volume;
	for (unsigned int index = 0; index < volume; index++) {
		weights[index] = 1./volume;
	}
	pixels = new Pixel[volume];
}

template<typename Pixel>
DownSamplingAdapter<Pixel>::~DownSamplingAdapter() {
	delete[] weights;
	delete[] pixels;
}

template<typename Pixel>
Pixel	DownSamplingAdapter<Pixel>::pixel(unsigned int x,
	unsigned int y) const {
	unsigned int	originx = x * sampling.width();
	unsigned int	originy = y * sampling.height();
	//Pixel	pixels[volume];
	unsigned int	index = 0;
	for (unsigned int dx = 0; dx < sampling.width(); dx++) {
		for (unsigned int dy = 0; dy < sampling.height(); dy++) {
			pixels[index++]
				= image.pixel(originx + dx, originy + dy);
		}
	}
	return weighted_sum(index, weights, pixels);
}

ImagePtr	downsample(ImagePtr image, const ImageSize& sampling);

template<typename Pixel>
class UpSamplingAdapter : public ConstImageAdapter<Pixel> {
	const ConstImageAdapter<Pixel>&	image;
	ImageSize	sampling;
public:
	UpSamplingAdapter(const ConstImageAdapter<Pixel>& image,
		const ImageSize& sampling);
	Pixel	pixel(unsigned int x, unsigned int y) const;
};

template<typename Pixel>
UpSamplingAdapter<Pixel>::UpSamplingAdapter(
	const ConstImageAdapter<Pixel>& _image, const ImageSize& _sampling)
	: ConstImageAdapter<Pixel>(
		ImageSize(_image.getSize().width() * _sampling.width(),
			_image.getSize().height() * _sampling.height())),
	  image(_image), sampling(_sampling) {
}

template<typename Pixel>
Pixel	UpSamplingAdapter<Pixel>::pixel(unsigned int x,
	unsigned int y) const {
	return image.pixel(x / sampling.width(), y / sampling.height());
}

ImagePtr	upsample(ImagePtr image, const ImageSize& sampling);

//////////////////////////////////////////////////////////////////////
// Luminance Adapter
//////////////////////////////////////////////////////////////////////
template<typename Pixel, typename T>
class LuminanceAdapter : public ConstImageAdapter<T> {
	const ConstImageAdapter<Pixel>&	image;
public:
	LuminanceAdapter(const ConstImageAdapter<Pixel>& image);
	T	pixel(unsigned int x, unsigned int y) const;
};

template<typename Pixel, typename T>
LuminanceAdapter<Pixel, T>::LuminanceAdapter(
	const ConstImageAdapter<Pixel>& _image)
	: ConstImageAdapter<T>(_image.getSize()), image(_image) {
}

template<typename Pixel, typename T>
T	LuminanceAdapter<Pixel, T>::pixel(unsigned int x,
			unsigned int y) const {
	T	v = luminance(image.pixel(x, y));
	return v;
}

//////////////////////////////////////////////////////////////////////
// Rescaling adapter
//////////////////////////////////////////////////////////////////////
template<typename Pixel>
class RescalingAdapter : public ConstImageAdapter<Pixel> {
	const ConstImageAdapter<Pixel>&	image;
	double	minpixel;
	double	scale;
	Pixel	zero;
public:
	RescalingAdapter(const ConstImageAdapter<Pixel>& image,
		double _minpixel, double scale);
	virtual Pixel	pixel(unsigned int x, unsigned int y) const;
};

template<typename Pixel>
RescalingAdapter<Pixel>::RescalingAdapter(const ConstImageAdapter<Pixel>& _image,
		double _minpixel, double _scale)
		: ConstImageAdapter<Pixel>(_image.getSize()), image(_image),
		  minpixel(_minpixel), scale(_scale), zero(minpixel) {
}

template<typename Pixel>
Pixel	RescalingAdapter<Pixel>::pixel(unsigned int x, unsigned int y)
	const {
	return (image.pixel(x, y) - zero) * scale;
}

//////////////////////////////////////////////////////////////////////
// PixelValue adapter, works for any image and returns float or double
// result types
//////////////////////////////////////////////////////////////////////
template<typename Pixel>
class ConstPixelValueAdapter : public ConstImageAdapter<Pixel> {
	const Image<unsigned char>	*byteimage;
	const Image<unsigned short>	*shortimage;
	const Image<unsigned int>	*intimage;
	const Image<unsigned long>	*longimage;
	const Image<float>		*floatimage;
	const Image<double>		*doubleimage;
public:
	ConstPixelValueAdapter(const ImagePtr& image);
	virtual Pixel	pixel(unsigned int x, unsigned int y) const;
};

template<typename Pixel>
ConstPixelValueAdapter<Pixel>::ConstPixelValueAdapter(const ImagePtr& image)
	: ConstImageAdapter<Pixel>(image->size()) {
	byteimage = dynamic_cast<Image<unsigned char> *>(&*image);
	shortimage = dynamic_cast<Image<unsigned short> *>(&*image);
	intimage = dynamic_cast<Image<unsigned int> *>(&*image);
	longimage = dynamic_cast<Image<unsigned long> *>(&*image);
	floatimage = dynamic_cast<Image<float> *>(&*image);
	doubleimage = dynamic_cast<Image<double> *>(&*image);
	if ((NULL == byteimage) &&
	    (NULL == shortimage) &&
	    (NULL == intimage) &&
	    (NULL == longimage) &&
	    (NULL == floatimage) &&
	    (NULL == doubleimage)) {
		throw std::runtime_error("pixel type not primitive");
	}
}

template <typename Pixel>
Pixel	ConstPixelValueAdapter<Pixel>::pixel(unsigned int x, unsigned int y) const {
	if (byteimage) {   return byteimage->pixelvalue<Pixel>(x, y);   }
	if (shortimage) {  return shortimage->pixelvalue<Pixel>(x, y);  }
	if (intimage) {    return intimage->pixelvalue<Pixel>(x, y);    }
	if (longimage) {   return longimage->pixelvalue<Pixel>(x, y);   }
	if (floatimage) {  return floatimage->pixelvalue<Pixel>(x, y);  }
	if (doubleimage) { return doubleimage->pixelvalue<Pixel>(x, y); }
	if (std::numeric_limits<Pixel>::has_quiet_NaN) {
		return std::numeric_limits<Pixel>::quiet_NaN();
	}
	throw std::runtime_error("NaN not available");
}

template <typename Pixel>
class PixelValueAdapter : public ConstImageAdapter<Pixel> {
	const Image<unsigned char>	*byteimage;
	const Image<unsigned short>	*shortimage;
	const Image<unsigned int>	*intimage;
	const Image<unsigned long>	*longimage;
	const Image<float>		*floatimage;
	const Image<double>		*doubleimage;
public:
	PixelValueAdapter(ImagePtr& image);
	virtual Pixel	pixel(unsigned int x, unsigned int y) const;
};

template<typename Pixel>
PixelValueAdapter<Pixel>::PixelValueAdapter(ImagePtr& image) :
	ConstImageAdapter<Pixel>(image->size()) {
	byteimage = dynamic_cast<Image<unsigned char> *>(&*image);
	shortimage = dynamic_cast<Image<unsigned short> *>(&*image);
	intimage = dynamic_cast<Image<unsigned int> *>(&*image);
	longimage = dynamic_cast<Image<unsigned long> *>(&*image);
	floatimage = dynamic_cast<Image<float> *>(&*image);
	doubleimage = dynamic_cast<Image<double> *>(&*image);
	if ((NULL == byteimage) &&
	    (NULL == shortimage) &&
	    (NULL == intimage) &&
	    (NULL == longimage) &&
	    (NULL == floatimage) &&
	    (NULL == doubleimage)) {
		throw std::runtime_error("pixel type not primitive");
	}
}

template<typename Pixel>
Pixel	PixelValueAdapter<Pixel>::pixel(unsigned int x, unsigned int y) const {
        if (byteimage) {   return byteimage->pixelvalue<Pixel>(x, y);   }
        if (shortimage) {  return shortimage->pixelvalue<Pixel>(x, y);  }
        if (intimage) {    return intimage->pixelvalue<Pixel>(x, y);    }
        if (longimage) {   return longimage->pixelvalue<Pixel>(x, y);   }
        if (floatimage) {  return floatimage->pixelvalue<Pixel>(x, y);  }
        if (doubleimage) { return doubleimage->pixelvalue<Pixel>(x, y); }
        if (std::numeric_limits<Pixel>::has_quiet_NaN) {
                return std::numeric_limits<Pixel>::quiet_NaN();
        }
        throw std::runtime_error("NaN not available");
}

//////////////////////////////////////////////////////////////////////
// RGB Adapter
//////////////////////////////////////////////////////////////////////
template<typename T>
class RGBAdapter : public ConstImageAdapter<RGB<double> > {
	const ConstImageAdapter<RGB<T> >&	image;
public:
	RGBAdapter(const ConstImageAdapter<RGB<T> >& image);
	RGB<double>	pixel(unsigned int x, unsigned int y) const;
};

template<typename T>
RGBAdapter<T>::RGBAdapter(const ConstImageAdapter<RGB<T> >& _image)
	: ConstImageAdapter<RGB<double> >(_image.getSize()), image(_image) {
}

template<typename T>
RGB<double>	RGBAdapter<T>::pixel(unsigned int x, unsigned int y) const {
	return RGB<double>(image.pixel(x, y));
}

//////////////////////////////////////////////////////////////////////
// Color adapters
//////////////////////////////////////////////////////////////////////
template<typename T>
class ColorAdapter : public ConstImageAdapter<T> {
protected:
	const ConstImageAdapter<RGB<T> >&	_image;
public:
	ColorAdapter(const ConstImageAdapter<RGB<T> >& image)
		: ConstImageAdapter<T>(image.getSize()), _image(image) { }
	virtual T	pixel(unsigned int x, unsigned int y) const {
		T	v = _image.pixel(x, y).luminance();
		return v;
	}
};

template<typename T>
class ColorRedAdapter : public ColorAdapter<T> {
public:
	using ColorAdapter<T>::_image;
	ColorRedAdapter(const ConstImageAdapter<RGB<T> >& image)
		: ColorAdapter<T>(image) { }
	virtual T	pixel(unsigned int x, unsigned int y) const {
		return _image.pixel(x, y).R;
	}
};

template<typename T>
class ColorGreenAdapter : public ColorAdapter<T> {
public:
	using ColorAdapter<T>::_image;
	ColorGreenAdapter(const ConstImageAdapter<RGB<T> >& image)
		: ColorAdapter<T>(image) { }
	virtual T	pixel(unsigned int x, unsigned int y) const {
		return _image.pixel(x, y).G;
	}
};

template<typename T>
class ColorBlueAdapter : public ColorAdapter<T> {
public:
	using ColorAdapter<T>::_image;
	ColorBlueAdapter(const ConstImageAdapter<RGB<T> >& image)
		: ColorAdapter<T>(image) { }
	virtual T	pixel(unsigned int x, unsigned int y) const {
		return _image.pixel(x, y).B;
	}
};

template<typename T>
class ColorMaxAdapter : public ColorAdapter<T> {
public:
	using ColorAdapter<T>::_image;
	ColorMaxAdapter(const ConstImageAdapter<RGB<T> >& image)
		: ColorAdapter<T>(image) { }
	virtual T	pixel(unsigned int x, unsigned int y) const {
		return _image.pixel(x, y).max();
	}
};

template<typename T>
class ColorMinAdapter : public ColorAdapter<T> {
public:
	using ColorAdapter<T>::_image;
	ColorMinAdapter(const ConstImageAdapter<RGB<T> >& image)
		: ColorAdapter<T>(image) { }
	virtual T	pixel(unsigned int x, unsigned int y) const {
		return _image.pixel(x, y).min();
	}
};

//////////////////////////////////////////////////////////////////////
// YUYV-Adapter
//////////////////////////////////////////////////////////////////////
template<typename T>
class YUYVAdapter : public ConstImageAdapter<RGB<T> > {
	const ConstImageAdapter<YUYV<T> >&	image;
public:
	YUYVAdapter(const ConstImageAdapter<YUYV<T> >& image);
	virtual RGB<T>	pixel(unsigned int x, unsigned int y) const;
};

template<typename T>
YUYVAdapter<T>::YUYVAdapter(const ConstImageAdapter<YUYV<T> >& _image)
	: ConstImageAdapter<RGB<T> >(_image.getSize()), image(_image) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "YUYVAdapter of size %s constructed",
		this->getSize().toString().c_str());
}

template<typename T>
RGB<T>	YUYVAdapter<T>::pixel(unsigned int x, unsigned int y) const {
	// get the pixel pair
	unsigned int	pairx = x - (x % 2);
	YUYV<T>	yuyvpixels[2];
	yuyvpixels[0] = image.pixel(pairx    , y);
	yuyvpixels[1] = image.pixel(pairx + 1, y);

	// convert the pair to RGB
	RGB<T>	rgbpixels[2];
	convertPixelPair(rgbpixels, yuyvpixels);

#if 0
	debug(LOG_DEBUG, DEBUG_LOG, 0, "%4u,%4 = %3u,%3u -> %3u,%3u,%3u", x, y,
		yuyvpixels[x % 2].y, yuyvpixels[x % 2].uv,
		rgbpixels[x % 2].R, rgbpixels[x % 2].G, rgbpixels[x % 2].B);
#endif

	// extract the "right" RGB pixel
	return rgbpixels[x % 2];
}

//////////////////////////////////////////////////////////////////////
// Function adapter
//////////////////////////////////////////////////////////////////////
template <typename Pixel>
class FunctionAdapter : public ConstImageAdapter<double> {
	const ConstImageAdapter<Pixel>&	image;
	double	(*f)(double);
public:
	FunctionAdapter(const ConstImageAdapter<Pixel>& _image,
		double (*_f)(double))
		: ConstImageAdapter<double>(_image.getSize()),
		  image(_image), f(_f) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "creating function adapter");
	}
	virtual double	pixel(unsigned int x, unsigned int y) const {
		double	v = image.pixel(x, y);
		return f(v);
	}
};

/**
 * \brief Adapter to square all the pixel values of an image
 *
 * This could be implemented with a Function adapter, but this special
 * case can probably be handled more efficiently
 */
template<typename Pixel>
class SquareAdapter : public ConstImageAdapter<double> {
	const ConstImageAdapter<Pixel>&	image;
public:
	SquareAdapter(const ConstImageAdapter<Pixel>& _image)
		: ConstImageAdapter<double>(_image.getSize()), image(_image) { }
	virtual double	pixel(unsigned int x, unsigned int y) const {
		double	v = image.pixel(x, y);
		return v * v;
	}
};

/**
 * \brief Adapter to perform a mirror image
 */
template<typename Pixel>
class MirrorAdapter : public ConstImageAdapter<Pixel> {
public:
	typedef enum { NONE, HORIZONTAL, VERTICAL, CENTRAL } symmetry;
private:
	const ConstImageAdapter<Pixel>&	image;
	symmetry	direction;
public:
	MirrorAdapter(const ConstImageAdapter<Pixel>& _image,
		const symmetry& _direction)
		: ConstImageAdapter<Pixel>(_image.getSize()), image(_image),
		  direction(_direction) {
	}
	virtual Pixel	pixel(unsigned int x, unsigned int y) const {
		
		switch (direction) {
		case NONE:
			break;
		case HORIZONTAL:
			x = image.getSize().width() - x;
			break;
		case CENTRAL:
			x = image.getSize().width() - x;
		case VERTICAL:
			y = image.getSize().height() - y;
			break;
		}
		return image.pixel(x, y);
	}
};

/**
 * \brief Adapter to create a Bayer mosaic image from an RGB image
 */
template<typename Pixel>
class MosaicAdapter : public ConstImageAdapter<Pixel> {
	const ConstImageAdapter<RGB<Pixel> >&	image;
	MosaicType	mosaic;
public:
	MosaicAdapter(const ConstImageAdapter<RGB<Pixel> >& _image,
		const MosaicType& _mosaic);
	virtual Pixel	pixel(unsigned int x, unsigned int y) const;
};

template<typename Pixel>
MosaicAdapter<Pixel>::MosaicAdapter(
	const ConstImageAdapter<RGB<Pixel> >& _image, const MosaicType& _mosaic)
	: ConstImageAdapter<Pixel>(_image.getSize()), image(_image),
	  mosaic(_mosaic) {
}

template<typename Pixel>
Pixel	MosaicAdapter<Pixel>::pixel(unsigned int x, unsigned int y) const {
	if (mosaic.isR(x, y)) {
		return image.pixel(x, y).R;
	}
	if (mosaic.isG(x, y)) {
		return image.pixel(x, y).G;
	}
	if (mosaic.isB(x, y)) {
		return image.pixel(x, y).B;
	}
}

//////////////////////////////////////////////////////////////////////
// Functor adapter
//////////////////////////////////////////////////////////////////////
template<typename Functor>
class FunctorAdapter : public ConstImageAdapter<double> {
	const ConstImageAdapter<double>&	image;
	Functor	f;
public:
	FunctorAdapter(const ConstImageAdapter<double>& _image,
		const Functor& _f)
		: ConstImageAdapter<double>(_image.getSize()),
		  image(_image), f(_f) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "creating functor adapter");
	}
	virtual double	pixel(unsigned int x, unsigned int y) const {
		return f(image.pixel(x,y));
	}
};

//////////////////////////////////////////////////////////////////////
// Window scaling adapter
//////////////////////////////////////////////////////////////////////
/**
 * \brief Quick and dirty adapter to extract a subrectangle and change scale
 *
 * This adapter does not attempt to interpolate pixels, it just computs
 * the coordinates and rounds them down
 */
template<typename Pixel>
class WindowScalingAdapter : public ConstImageAdapter<Pixel> {
	const ConstImageAdapter<Pixel>&	_image;
	ImageRectangle	_source;
	double	xscaling;
	double	yscaling;
public:
	WindowScalingAdapter(const ConstImageAdapter<Pixel>& image,
		const ImageRectangle& source, const ImageSize& target)
		: ConstImageAdapter<Pixel>(target), _image(image),
		  _source(source) {
		xscaling = _source.size().width() / (double)target.width();
		yscaling = _source.size().height() / (double)target.height();
debug(LOG_DEBUG, DEBUG_LOG, 0, "xscaling = %f, yscaling = %f", xscaling, yscaling);
	}
	virtual Pixel	pixel(unsigned int x, unsigned int y) const {
		unsigned int	xx = trunc(_source.origin().x() + xscaling * x);
		unsigned int	yy = trunc(_source.origin().y() + yscaling * y);
		return _image.pixel(xx, yy);
	}
};

//////////////////////////////////////////////////////////////////////
// Level detection adapter
//////////////////////////////////////////////////////////////////////
/**
 * \brief Adapter to create a mask of pixels exceeding a value
 *
 */
template<typename Pixel>
class LevelMaskAdapter : public ConstImageAdapter<unsigned char> {
	const ConstImageAdapter<Pixel>&	_image;
	double	_level;
public:
	LevelMaskAdapter(const ConstImageAdapter<Pixel>& image,
		const double level) :
		ConstImageAdapter<unsigned char>(image.getSize()),
		_image(image), _level(level) {
	}
	virtual unsigned char	pixel(unsigned int x, unsigned int y) const {
		return (_image.pixel(x, y) >= _level) ? 1 : 0;
	}
};

/**
 * \brief Extract a mask from an image
 */
class LevelMaskExtractor {
	double	_level;
public:
	LevelMaskExtractor(double level) : _level(level) { }
	ImagePtr	operator()(const ImagePtr image) const;
};

//////////////////////////////////////////////////////////////////////
// various focus measure adapters
//////////////////////////////////////////////////////////////////////
template<typename Pixel>
class HorizontalGradientAdapter : public ConstImageAdapter<double> {
	const ConstImageAdapter<Pixel>&	image;
public:
	HorizontalGradientAdapter(const ConstImageAdapter<Pixel>& _image)
		: ConstImageAdapter<double>(_image.getSize()), image(_image) {
	}
	virtual double	pixel(unsigned int x, unsigned int y) const {
		if (x >= (image.getSize().width() - 1)) {
			return 0;
		}
		double	dx = (double)image.pixel(x + 1, y)
				- (double)image.pixel(x, y);
		return dx * dx;
	}
};

template<typename Pixel>
class VerticalGradientAdapter : public ConstImageAdapter<double> {
	const ConstImageAdapter<Pixel>&	image;
public:
	VerticalGradientAdapter(const ConstImageAdapter<Pixel>& _image)
		: ConstImageAdapter<double>(_image.getSize()), image(_image) {
	}
	virtual double	pixel(unsigned int x, unsigned int y) const {
		if (y >= (image.getSize().height() - 1)) {
			return 0;
		}
		double	dy = (double)image.pixel(x, y + 1)
				- (double)image.pixel(x, y);
		return dy * dy;
	}
};

template<typename Pixel>
class SquaredGradientAdapter : public ConstImageAdapter<double> {
	const ConstImageAdapter<Pixel>&	image;
public:
	SquaredGradientAdapter(const ConstImageAdapter<Pixel>& _image)
		: ConstImageAdapter<double>(_image.getSize()), image(_image) {
	}
	virtual double	pixel(unsigned int x, unsigned int y) const {
		if (x >= (image.getSize().width() - 1)) {
			return 0;
		}
		if (y >= (image.getSize().height() - 1)) {
			return 0;
		}
		double	dx = (double)image.pixel(x + 1, y)
				- (double)image.pixel(x, y);
		double	dy = (double)image.pixel(x, y + 1)
				- (double)image.pixel(x, y);
		return dx * dx + dy * dy;
	}
};

template<typename Pixel>
class HorizontalBrennerAdapter : public ConstImageAdapter<double> {
	const ConstImageAdapter<Pixel>&	image;
public:
	HorizontalBrennerAdapter(const ConstImageAdapter<Pixel>& _image)
		: ConstImageAdapter<double>(_image.getSize()), image(_image) {
	}
	virtual double	pixel(unsigned int x, unsigned int y) const {
		if (x > (image.getSize().width() - 2)) {
			return 0;
		}
		double	dx = (double)image.pixel(x + 2, y)
				- (double)image.pixel(x, y);
		return dx * dx;
	}
};

template<typename Pixel>
class VerticalBrennerAdapter : public ConstImageAdapter<double> {
	const ConstImageAdapter<Pixel>&	image;
public:
	VerticalBrennerAdapter(const ConstImageAdapter<Pixel>& _image)
		: ConstImageAdapter<double>(_image.getSize()), image(_image) {
	}
	virtual double	pixel(unsigned int x, unsigned int y) const {
		if (y > (image.getSize().height() - 2)) {
			return 0;
		}
		double	dy = (double)image.pixel(x, y + 2)
			- (double)image.pixel(x, y);
		return dy * dy;
	}
};

template<typename Pixel>
class BrennerAdapter : public ConstImageAdapter<double> {
	const ConstImageAdapter<Pixel>&	image;
public:
	BrennerAdapter(const ConstImageAdapter<Pixel>& _image)
		: ConstImageAdapter<double>(_image.getSize()), image(_image) {
	}
	virtual double	pixel(unsigned int x, unsigned int y) const {
		if (x > (image.getSize().width() - 2)) {
			return 0;
		}
		if (y > (image.getSize().height() - 2)) {
			return 0;
		}
		double	dx = (double)image.pixel(x + 2, y)
				- (double)image.pixel(x, y);
		double	dy = (double)image.pixel(x, y + 2)
			- (double)image.pixel(x, y);
		return dx * dx + dy * dy;
	}
};

//////////////////////////////////////////////////////////////////////
// Adapter to combine several images into a single color image
//////////////////////////////////////////////////////////////////////
template<typename Pixel>
class CombinationAdapter : public ConstImageAdapter<RGB<Pixel> > {
	const ConstImageAdapter<Pixel>&	_red;
	const ConstImageAdapter<Pixel>&	_green;
	const ConstImageAdapter<Pixel>&	_blue;
public:
	CombinationAdapter(const ConstImageAdapter<Pixel>& red,
		const ConstImageAdapter<Pixel>& green,
		const ConstImageAdapter<Pixel>& blue)
		: ConstImageAdapter<RGB<Pixel> >(red.getSize()),
		  _red(red), _green(green), _blue(blue) {
		if ((red.getSize() != green.getSize())
			|| (red.getSize() != blue.getSize())) {
			throw std::runtime_error("image sizes don't match");
		}
	}
	virtual RGB<Pixel>	pixel(unsigned int x, unsigned int y) const {
		Pixel	r = _red.pixel(x, y);
		Pixel	g = _green.pixel(x, y);
		Pixel	b = _blue.pixel(x, y);
		return RGB<Pixel>(r, g, b);
	}
};

//////////////////////////////////////////////////////////////////////
// Adapter to draw crosshairs at a point
//////////////////////////////////////////////////////////////////////
template<typename Pixel>
class CrosshairAdapter : public ConstImageAdapter<Pixel> {
	ImagePoint	where;
	int	length;
public:
	CrosshairAdapter(const ImageSize& _size, const ImagePoint& _where,
		int _length = 3)
		: ConstImageAdapter<Pixel>(_size), where(_where),
		  length(_length) { }
	Pixel	pixel(unsigned int x, unsigned int y) const {
		int	deltax = x - where.x();
		int	deltay = y - where.y();
		if ((deltax != 0) && (deltay != 0)) {
			return 0;
		}
		if ((deltax == 0) && (abs(deltay) < length)) {
			return std::numeric_limits<Pixel>::max();
		}
		if ((deltay == 0) && (abs(deltax) < length)) {
			return std::numeric_limits<Pixel>::max();
		}
		return 0;
	}
};

//////////////////////////////////////////////////////////////////////
// Adapter to draw a circle at a point with a given radius
//////////////////////////////////////////////////////////////////////
template<typename Pixel>
class CircleAdapter : public ConstImageAdapter<Pixel> {
	double	radius;
	ImagePoint	center;
public:
	CircleAdapter(const ImageSize& _size, const ImagePoint& _center,
		double _radius) : ConstImageAdapter<Pixel>(_size),
				radius(_radius), center(_center) {
	}
	Pixel	pixel(unsigned int x, unsigned int y) const {
		if (hypot((double)x - (double)center.x(),
			(double)y - (double)center.y()) <= radius) {
			return std::numeric_limits<Pixel>::max() / 2;
		}
		return 0;
	}
};

//////////////////////////////////////////////////////////////////////
// Min/Maximum adapter for two images
//////////////////////////////////////////////////////////////////////
template<typename Pixel>
class MaxAdapter : public ConstImageAdapter<Pixel> {
	const ConstImageAdapter<Pixel>&	first;
	const ConstImageAdapter<Pixel>&	second;
public:
	MaxAdapter(const ConstImageAdapter<Pixel>& _first,
		const ConstImageAdapter<Pixel>& _second)
		: ConstImageAdapter<Pixel>(_first.getSize()),
		  first(_first), second(_second) {
		if (first.getSize() != second.getSize()) {
			throw std::runtime_error("images have different size");
		}
	}
	Pixel	pixel(unsigned int x, unsigned int y) const {
		Pixel	v1 = first.pixel(x, y);
		Pixel	v2 = second.pixel(x, y);
		if (v1 > v2) {
			return v1;
		}
		return v2;
	}
};

template<typename Pixel>
class MinAdapter : public ConstImageAdapter<Pixel> {
	const ConstImageAdapter<Pixel>&	first;
	const ConstImageAdapter<Pixel>&	second;
public:
	MinAdapter(const ConstImageAdapter<Pixel>& _first,
		const ConstImageAdapter<Pixel>& _second)
		: ConstImageAdapter<Pixel>(_first.getSize()),
		  first(_first), second(_second) {
		if (first.getSize() != second.getSize()) {
			throw std::runtime_error("images have different size");
		}
	}
	Pixel	pixel(unsigned int x, unsigned int y) const {
		Pixel	v1 = first.pixel(x, y);
		Pixel	v2 = second.pixel(x, y);
		if (v1 > v2) {
			return v2;
		}
		return v1;
	}
};

//////////////////////////////////////////////////////////////////////
// Rescale an image to a given value
// (duplicates but also simplifies RescalingAdapter)
//////////////////////////////////////////////////////////////////////
template<typename Pixel>
class RescaleAdapter : public ConstImageAdapter<Pixel> {
	const ConstImageAdapter<Pixel>& image;
	double	multiplier;
public:
	RescaleAdapter(const ConstImageAdapter<Pixel>& _image, double maxvalue)
		: ConstImageAdapter<Pixel>(_image.getSize()), image(_image) {
		if (std::numeric_limits<Pixel>::is_integer) {
			multiplier = std::numeric_limits<Pixel>::max() / maxvalue;
		} else {
			multiplier = 1 / maxvalue;
		}
		debug(LOG_DEBUG, DEBUG_LOG, 0,
			"create rescale with multiplier %g", multiplier);
	}

	Pixel	pixel(unsigned int x, unsigned int y) const {
		Pixel	v = image.pixel(x, y) * multiplier;
		return v;
	}
};

//////////////////////////////////////////////////////////////////////
// An adapter that returns an image of uniform value
//////////////////////////////////////////////////////////////////////
template<typename Pixel>
class ConstantValueAdapter : public ConstImageAdapter<Pixel> {
	Pixel	value;
public:
	ConstantValueAdapter(const ImageSize& _size, Pixel _value)
		: ConstImageAdapter<Pixel>(_size), value(_value) { }
	Pixel	pixel(unsigned int /* x */, unsigned int /* y */) const {
		return value;
	}

};

//////////////////////////////////////////////////////////////////////
// Type conversion
//////////////////////////////////////////////////////////////////////
template<typename Pixel, typename srcPixel>
class TypeConversionAdapter : public ConstImageAdapter<Pixel> {
	const ConstImageAdapter<srcPixel>&	image;
public:
	TypeConversionAdapter(const ConstImageAdapter<srcPixel>& _image)
		: ConstImageAdapter<Pixel>(_image.getSize()), image(_image) { }
	Pixel	pixel(unsigned int x, unsigned int y) const {
		Pixel	result;
		if ((std::numeric_limits<Pixel>::is_integer)
			&& (!std::numeric_limits<srcPixel>::is_integer)) {
			convertPixel(result, std::numeric_limits<Pixel>::max()
						* image.pixel(x, y));
		} else if ((!std::numeric_limits<Pixel>::is_integer)
			&& (std::numeric_limits<srcPixel>::is_integer)) {
			convertPixel(result, image.pixel(x, y) /
				(Pixel)std::numeric_limits<srcPixel>::max());
		} else {
			convertPixel(result, image.pixel(x, y));
		}
		return result;
	}
};

class DoubleAdapter : public ConstImageAdapter<double> {
	const ImagePtr	_image;
	std::shared_ptr<ConstImageAdapter<double> >	doubleimage;
public:
	DoubleAdapter(const ImagePtr image);
	virtual double	pixel(unsigned int x, unsigned int y) const {
		return doubleimage->pixel(x, y);
	}
};

#if 0
//////////////////////////////////////////////////////////////////////
// Pixel interpolation adapter
//////////////////////////////////////////////////////////////////////
template<typename Pixel>
class PixelInterpolationAdapter : public ConstImageAdapter<Pixel> {
	const ConstImageAdapter<Pixel>&	image;
public:
	PixelInterpolationAdapter(const ImageSize& size,
		const ConstImageAdapter<Pixel>& _image)
		: ConstImageAdapter<Pixel>(size), image(_image) {
	}
	Pixel	pixel(const astro::Point& t) const {
		// find out in which pixel this is located
		int     tx = floor(t.x());
		int     ty = floor(t.y());

		// compute the weights
		double  wx = t.x() - tx;
		double  wy = t.y() - ty;

		// compute the weights
		double  weights[4];
		weights[0] = (1 - wx) * (1 - wy);
		weights[1] = wx * (1 - wy);
		weights[2] = (1 - wx) * wy;
		weights[3] = wx * wy;

		// now compute the weighted sum of the pixels
		Pixel   a[4];
		ImageSize       size = image.getSize();

		// lower left corner
		if (size.contains(tx    , ty    )) {
			a[0] = image.pixel(tx    , ty    );
		} else {
			a[0] = Pixel(0);
		}
		// lower right corner
		if (size.contains(tx + 1, ty    )) {
			a[1] = image.pixel(tx + 1, ty    );
		} else {
			a[1] = Pixel(0);
		}
		// upper left corner
		if (size.contains(tx    , ty + 1)) {
			a[2] = image.pixel(tx    , ty + 1);
		} else {
			a[2] = Pixel(0);
		}
		// upper right corner
		if (size.contains(tx + 1, ty + 1)) {
			a[3] = image.pixel(tx + 1, ty + 1);
		} else {
			a[3] = Pixel(0);
		}
		return weighted_sum(4, weights, a);
	}
};
#endif

} // namespace adapter
} // namespace astro

#endif /* _AstroAdapter_h */
