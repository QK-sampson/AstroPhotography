/*
 * ImageCalibrationStep.cpp -- Calibration of raw images
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <AstroProcess.h>
#include <AstroDebug.h>
#include <AstroFormat.h>
#include <algorithm>

using namespace astro::image;
using namespace astro::adapter;

namespace astro {
namespace process {

/**
 * \brief Create an Image Calibration step
 */
ImageCalibrationStep::ImageCalibrationStep() {
}

/**
 * \brief Destroy an Image Calibration step
 */
ImageCalibrationStep::~ImageCalibrationStep() {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "image calibration step deleted");
}

/**
 * \brief Auxiliary function to search the precursors for a calibration image
 */
const CalibrationImageStep	*ImageCalibrationStep::calimage(
				CalibrationImageStep::caltype t) const {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "looking for a precursor of type %s "
		"among %d precursors",
		CalibrationImageStep::caltypename(t).c_str(),
		precursors().size());

	ProcessingStep::steps::const_iterator	i
		= std::find_if(precursors().begin(), precursors().end(),
		// lambda predicate to find precursor steps that are 
		// calibration images and of the correct type
		[t](const ProcessingStep *step) {
			// check whether its a calibration image
			const CalibrationImageStep	*image
				= dynamic_cast<const CalibrationImageStep *>(step);
			if (image == NULL) {
				return false;
			}
			// check whether the type matches
			return (t == image->type());
		}
	);

	// find out whether the find_if algorithm did find a suitable precursor
	if (precursors().end() == i) {
		throw std::runtime_error(
			stringprintf("no precursor of type %s found",
				CalibrationImageStep::caltypename(t).c_str()));
	}
	const CalibrationImageStep	*result
		= dynamic_cast<const CalibrationImageStep *>(*i);
	if (NULL == result) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "precursor is not a "
			"calibration image");
		throw std::runtime_error("precursor is not a calibration image");
	}
	debug(LOG_DEBUG, DEBUG_LOG, 0, "found precursor in state %s",
		ProcessingStep::statename(result->status()).c_str());
	return result;
}

/**
 * \brief Adapter to perform the calibration for full size images
 */
class CalibrationAdapter : public ConstImageAdapter<double> {
protected:
	const CalibrationImageStep *_dark;
	const CalibrationImageStep *_flat;
	const ConstImageAdapter<double>& _image;
	virtual double	darkpixel(unsigned int x, unsigned int y) const {
		return _dark->out().pixel(x, y);
	}
	virtual double	flatpixel(unsigned int x, unsigned int y) const {
		return _flat->out().pixel(x, y);
	}
public:
	CalibrationAdapter(
		const CalibrationImageStep *dark,
		const CalibrationImageStep *flat,
		const ConstImageAdapter<double>& image)
		: ConstImageAdapter<double>(image.getSize()),
		  _dark(dark), _flat(flat), _image(image) { }

	virtual double	pixel(unsigned int x, unsigned int y) const {
		double	value = _image.pixel(x, y);
		if (_dark) {
			value -= darkpixel(x, y);
			if (value < 0) {
				value = 0;
			}
		}
		if (_flat) {
			value /= flatpixel(x, y);
		}
		return value;
	}
};

/**
 * \brief Adapter to perform the calibration for subrectangles
 */
class WindowedCalibrationAdapter : public CalibrationAdapter {
	ImageRectangle	_window;
	virtual double	darkpixel(unsigned int x, unsigned int y) const {
		return _dark->out().pixel(_window.origin().x() + x,
			_window.origin().y() + y);
	}
	virtual double	flatpixel(unsigned int x, unsigned int y) const {
		return _flat->out().pixel(_window.origin().x() + x,
			_window.origin().y() + y);
	}
public:
	WindowedCalibrationAdapter(
		const CalibrationImageStep *dark,
		const CalibrationImageStep *flat,
		const ConstImageAdapter<double>& image,
		const ImageRectangle& window) 
		: CalibrationAdapter(dark, flat, image), _window(window) {
		// verify that the window fits inside the calibration image
		// retangles
		if (NULL != dark) {
			if (!dark->out().getSize().bounds(_window)) {
				throw std::runtime_error("window does not fit "
					"in dark");
			}
		}
		if (NULL != flat) {
			if (!flat->out().getSize().bounds(_window)) {
				throw std::runtime_error("window does not fit "
					"in flat");
			}
		}
	}
};

/**
 * \brief Do the actual work
 *
 * Image calibration needs a dark image an a flat image, which it looks
 * for among the precursors. It then calibrates the remaining image
 */
ProcessingStep::state	ImageCalibrationStep::do_work() {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "perform image calibration");
	// scan precursors for a dark image
	const CalibrationImageStep	*dark = NULL;
	try {
		dark = calimage(CalibrationImageStep::DARK);
		debug(LOG_DEBUG, DEBUG_LOG, 0, "found a dark precursor: %s",
			dark->out().getSize().toString().c_str());
	} catch (...) { }

	// scan precursors for a flat image
	const CalibrationImageStep	*flat = NULL;
	try {
		flat = calimage(CalibrationImageStep::FLAT);
		debug(LOG_DEBUG, DEBUG_LOG, 0, "found a flat precursor: %s",
			flat->out().getSize().toString().c_str());
	} catch (...) { }

	// verify that the calibration images have the same size
	if ((NULL != dark) && (NULL != flat)) {
		if (dark->out().getSize() != flat->out().getSize()) {
			debug(LOG_DEBUG, DEBUG_LOG, 0, "calibration image "
				"sizes don't match");
			throw std::runtime_error("calibration image sizes "
				"don't match");
		}
	}

	// find an image that is different
	debug(LOG_DEBUG, DEBUG_LOG, 0, "looking for different image");
	ProcessingStep::steps::const_iterator	i
		= std::find_if(precursors().begin(), precursors().end(),
		[dark, flat](ProcessingStep *step) {
			if (NULL == dynamic_cast<ImageStep *>(step)) {
				return false;
			}
			debug(LOG_DEBUG, DEBUG_LOG, 0,
				"step = %p, dark = %p, flat = %p",
				step, dark, flat);
			return ((step != dark) && (step != flat));
		}
	);
	if (i == precursors().end()) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "no image to calibrate");
		return ProcessingStep::idle;
	}
	debug(LOG_DEBUG, DEBUG_LOG, 0, "found an image to calibrate");
	ImageStep	*image = dynamic_cast<ImageStep *>(*i);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "image to calibration: size=%s",
		image->out().getSize().toString().c_str());

	// now build the calibration adapter
	if (image->out().getSize() == dark->out().getSize()) {
		// create a calibration adapter
		_out = outPtr(new CalibrationAdapter(dark, flat, image->out()));
	} else {
		// the image size and the dark size don't agree, so we
		// assume that the image is actuall a subframe, so we can
		// calibrate with the appropriate subframe of the dark and
		// flat frames. For this to work, we have to get the subframe
		// size
		debug(LOG_DEBUG, DEBUG_LOG, 0, "looking for subframe info");
		// this case can only be handled if the precursor is a raw
		// image (which will normally be the case) and has metadata
		// indicating that this is a subrectangle image
		RawImageFileStep	*raw
			= dynamic_cast<RawImageFileStep *>(image);
		if (NULL == raw) {
			std::string	msg("not a RawImageFile, "
				"cannot get subframe info");
			debug(LOG_DEBUG, DEBUG_LOG, 0, "%s", msg.c_str());
			throw ProcessingStep::idle;
		}

		// since we now have subframe info, we can create a calibration
		// adapter for the subframe
		ImageRectangle	window = raw->subframe();
		_out = outPtr(new WindowedCalibrationAdapter(dark, flat,
				image->out(), window));
		debug(LOG_DEBUG, DEBUG_LOG, 0, "windowed calibration adapter "
			"for subframe %s created", window.toString().c_str());
	}
	return ProcessingStep::complete;
}

} // namespace process
} // namespace astro