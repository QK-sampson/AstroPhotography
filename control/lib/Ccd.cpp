/*
 * Ccd.cpp -- Ccd implementation
 *
 * (c) 2012 Prof Dr Andreas Mueller, Hochschule Rapperswil
 * $Id$
 */
#include <AstroCamera.h>
#include <AstroFormat.h>
#include <AstroDebug.h>
#include <AstroExceptions.h>
#include <includes.h>

using namespace astro::image;

namespace astro {
namespace camera {

//////////////////////////////////////////////////////////////////////
// CcdInfo implementation
//////////////////////////////////////////////////////////////////////

CcdInfo::CcdInfo(const std::string& name, const ImageSize& size, int _ccdid)
	: _name(name), _size(size), ccdid(_ccdid) {
	_shutter = false;
}

/**
 * \brief Get the CCD size.
 */
const astro::image::ImageSize&	CcdInfo::size() const {
	return _size;
}

/**
 * \brief Get a frame filling the CCD
 *
 * This method returns an image rectangle that fills the CCD. This can
 * be used to initialize the exposure object for the getExposure
 * method. Some cameras, like the UVC cameras, can only display full
 * frames, not subframes.
 */
const ImageRectangle	CcdInfo::getFrame() const {
	return ImageRectangle(ImagePoint(0, 0), _size);
}

/**
 * \brief add a binning mode
 */
void	CcdInfo::addMode(const Binning& mode) {
	binningmodes.insert(mode);
}

/**
 * \brief add a set of binning modes
 */
void	CcdInfo::addModes(const BinningSet& modes) {
	BinningSet::const_iterator	i;
	for (i = modes.begin(); i != modes.end(); i++) {
		addMode(*i);
	}
}

/**
 * \brief Get the binning modes available for this CCD.
 */
const BinningSet&	CcdInfo::modes() const {
	return binningmodes;
}

/**
 * \brief Get the name of this CCD.
 */
const std::string&	CcdInfo::name() const {
	return _name;
}

/**
 * \brief Get the CCD id for this CCD.
 *
 * This is the index into the array of CCDs this camera has.
 */
int	CcdInfo::getId() const {
	return ccdid;
}

/**
 * \brief Return a string representation.
 */
std::string	CcdInfo::toString() const {
	return stringprintf("%s: %dx%d,%s", _name.c_str(),
		_size.width(), _size.height(),
		binningmodes.toString().c_str());
}

std::ostream&	operator<<(std::ostream& out, const CcdInfo& ccdinfo) {
	return out << ccdinfo.toString();
}

/**
 * \brief Fit a rectangle inside a ccd
 */
ImageRectangle	CcdInfo::clipRectangle(const ImageRectangle& rectangle) const {
	if (_size.width() < rectangle.origin().x()) {
		throw std::runtime_error("image rectangle outside ccd");
	}
	if (_size.height() < rectangle.origin().y()) {
		throw std::runtime_error("image rectangle outside ccd");
	}
	unsigned int	w = rectangle.size().width();
	if ((rectangle.size().width() + rectangle.origin().x()) > _size.width()) {
		w = _size.width() - rectangle.origin().x();
	}
	unsigned int	h = rectangle.size().height();
	if ((rectangle.size().height() + rectangle.origin().y()) > _size.height()) {
		h = _size.height() - rectangle.origin().y();
	}
	return ImageRectangle(rectangle.origin(), ImageSize(w, h));
}

/**
 * \brief Get a centered rectangle of a given size
 */
ImageRectangle	CcdInfo::centeredRectangle(const ImageSize& s) const {
	unsigned int	w = s.width();
	unsigned int	h = s.height();
	if (w > _size.width()) {
		w = _size.width();
	}
	if (h > _size.height()) {
		h = _size.height();
	}
	unsigned int	xoffset = (_size.width() - w) / 2;
	unsigned int	yoffset = (_size.height() - h) / 2;
	return ImageRectangle(ImagePoint(xoffset, yoffset), ImageSize(w, h));
}

//////////////////////////////////////////////////////////////////////
// Ccd implementation
//////////////////////////////////////////////////////////////////////

/**
 * \brief Start an exposure
 *
 * Initiate an exposure. The base class method performs some common
 * sanity checks (e.g. it will not accept subframes that don't fit within
 * the CCD area), and it will reject requests if an exposure is already in
 * progress. Derived classes should override this methode, but they should
 * call this method as the first step in their implementation, because
 * this method also sets up the infrastructure for the wait method.
 */
void    Ccd::startExposure(const Exposure& _exposure) {
	// make sure we are in the right state, and only accept new exposures
	// in that state. This is important because if we change the
	// exposure member while an exposure is in progress, we may run into
	// trouble while doing the readout. 
	if (Exposure::idle != state) {
		debug(LOG_ERR, DEBUG_LOG, 0,
			"start exposure only in idle state");
		throw BadState("start exposure only in idle state");
	}

	// copy the exposure info
	exposure = _exposure;

	// if the size was not specified in the exposure, take the full
	// CCD size
        if (exposure.frame.size() == ImageSize(0, 0)) {
                exposure.frame = getInfo().getFrame();
        }
	debug(LOG_DEBUG, DEBUG_LOG, 0, "start exposure: %s -> %s",
		_exposure.toString().c_str(),
		exposure.toString().c_str());

	// check that the frame to be exposed fits into the CCD
        if (!info.size().bounds(exposure.frame)) {
		debug(LOG_ERR, DEBUG_LOG, 0, "exposure does not fit in ccd");
                throw BadParameter("exposure does not fit ccd");
        }

	// remember the start time of the exposure, this will be useful
	// if we later want to wait for the exposure to complete.
	time(&lastexposurestart);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "exposure started at %d",
		lastexposurestart);
}

/**
 * \brief Monitor progress of an exposure
 *
 * Find out whether an exposure is in progress. Optional method.
 */
Exposure::State Ccd::exposureStatus() {
	return state;
}

/**
 * \brief Cancel an exposure
 *
 * Note that some cameras cannot cancel an exposure other than by
 * resetting the camera, which will affect other CCDs of the same
 * camera as well. If you plan to implement this function for such
 * a camera,
 * make sure that you would usually read from the camera is also
 * stored locally so that it can be restored after the reset.
 */
void    Ccd::cancelExposure() {
	throw NotImplemented("cancelExposure not implemented");
}

/**
 * \brief Waiting for completion is generic (except possibly for UVC cameras)
 */
bool	Ccd::wait() {
	if ((Exposure::idle == state) || (Exposure::cancelling == state)) {
		debug(LOG_ERR, DEBUG_LOG, 0,
			"cannot wait: no exposure in progress");
		throw BadState("cannot wait: no exposure requested");
	}
	debug(LOG_DEBUG, DEBUG_LOG, 0, "waiting for exposure to complete");
	if (Exposure::exposing == state) {
		// has the exposure time already expired? If so, we wait at
		// least as the exposure time indicates
		debug(LOG_DEBUG, DEBUG_LOG, 0,
			"lastexposurestart: %d, exposuretime: %f",
			lastexposurestart, exposure.exposuretime);
		double	endtime = lastexposurestart;
		endtime += exposure.exposuretime;
		time_t	now = time(NULL);
		debug(LOG_DEBUG, DEBUG_LOG, 0, "now: %d", now);
		int	delta = endtime - now;
		debug(LOG_DEBUG, DEBUG_LOG, 0, "delta = %u", delta);
		if (delta > 0) {
			debug(LOG_DEBUG, DEBUG_LOG, 0, "wait for exposure time "
				"to expire: %u", delta);
			sleep(delta);
			debug(LOG_DEBUG, DEBUG_LOG, 0, "wait complete");
		}
		// now wait in 0.1 second intervals until either the exposure
		// completes or we have waited for 30 seconds
		double	step = 0.1;
		int	counter = 300;
		while ((counter-- > 0) && (Exposure::exposing == this->exposureStatus())) {
			usleep(step * 1000000);
			debug(LOG_DEBUG, DEBUG_LOG, 0, "wait %d", counter);
		}
		if (counter == 0) {
			debug(LOG_ERR, DEBUG_LOG, 0,
				"timeout waiting for exposure");
			// XXX bad things should happen
		}
	}
	debug(LOG_DEBUG, DEBUG_LOG, 0, "wait complete");
	return (Exposure::exposed == state);
}

/**
 * \brief Retrieve an image from the camera
 */
astro::image::ImagePtr	Ccd::getImage() {
	if (Exposure::exposed != state) {
		throw BadState("no exposed image to retrieve");
	}
	throw NotImplemented("getImage not implemented");
}

/**
 * \brief Retrieve a sequence of images from the camera
 *
 * The default implementation just performs multiple startExposure/getImage
 * calls. We reuse the same exposure structure for all calls.
 * \param imagecount	number of images to retrieve
 */
astro::image::ImageSequence	Ccd::getImageSequence(unsigned int imagecount) {
	astro::image::ImageSequence	result;
	unsigned int	k = 0;
	while (k < imagecount) {
		if (k > 0) {
			startExposure(exposure);
		}
		wait();
		result.push_back(getImage());
		debug(LOG_DEBUG, DEBUG_LOG, 0, "image %d retrieved", k);
		k++;
	}
	return result;
}

/**
 * \brief Retrieve Cooler, using the cache if retrieved befor
 */
CoolerPtr	Ccd::getCooler() {
	if (!cooler) {
		cooler = this->getCooler0();
	}
	return cooler;
}

/**
 * \brief Retrieve a cooler
 */
CoolerPtr	Ccd::getCooler0() {
	throw NotImplemented("thermoelectric cooler not implemented");
}

/**
 * \brief Retrieve the state of the shutter
 */
shutter_state	Ccd::getShutterState() {
	throw NotImplemented("camera has no shutter");
}

/**
 * \brief Set the state of the shutter
 */
void	Ccd::setShutterState(const shutter_state& state) {
	// always accept shutter open
	if (SHUTTER_OPEN == state) {
		return;
	}
	throw NotImplemented("camera has no shutter");
}

/**
 * \brief add exposure metadata
 */
void	Ccd::addExposureMetadata(ImageBase& image) const {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "adding exposure metadata");
	exposure.addToImage(image);
	std::cout << image;
}

/**
 * \brief add temperature metadata
 */
void	Ccd::addTemperatureMetadata(ImageBase& image) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "adding temperature metadata");
	// only if a cooler is available for this CCD
	if (hasCooler()) {
		CoolerPtr	cooler = getCooler();
		cooler->addTemperatureMetadata(image);
	}
}

/**
 * \brief add metadata
 */
void	Ccd::addMetadata(ImageBase& image) {
	this->addExposureMetadata(image);
	this->addTemperatureMetadata(image);
}

} // namespace camera
} // namespace astro
