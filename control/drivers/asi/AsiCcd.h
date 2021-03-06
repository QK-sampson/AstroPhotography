/*
 * AsiCcd.h -- ASI camera CCD driver class declaration
 *
 * (c) 2016 Prof Dr Andreas Müller, Hochschule Rapperswil
 */
#ifndef _AsiCcd_h
#define _AsiCcd_h

#include <AstroCamera.h>
#include <AsiCamera.hh>

namespace astro {
namespace camera {
namespace asi {

class AsiStream;

/**
 * \brief Implementation class for the CCD of an ASI camera
 */
class AsiCcd : public Ccd {
	AsiCamera&	_camera;
	bool	_hasCooler;
	std::string	imgtypename();
public:
	static std::string	imgtype2string(int imgtype);
public:
	AsiCcd(const CcdInfo&, AsiCamera& camera);
	virtual ~AsiCcd();

	void	setExposure(const Exposure& exposure);
	virtual void	startExposure(const Exposure& exposure);
	virtual CcdState::State	exposureStatus();
	virtual void	cancelExposure();
	virtual bool	wait();

	// image retrieval
	virtual astro::image::ImagePtr	getRawImage();

protected:
	virtual CoolerPtr	getCooler0();
public:
	void	hasCooler(bool hc) { _hasCooler = hc; }
	virtual bool	hasCooler() const { return _hasCooler; }

	// stream interface
private:
	AsiStream	*stream;
public:
	virtual void	streamExposure(const Exposure& exposure);
	virtual void	startStream(const Exposure& exposure);
	virtual void	stopStream();
	virtual bool	streaming();
};

} // namespace asi
} // namespace camera
} // namespace astro

#endif /* _AsiCcd_h */
