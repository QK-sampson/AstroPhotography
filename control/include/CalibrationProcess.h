/*
 * CalibrationProcess.h -- declaration of the CalibrationProcess class
 *
 * This class is not to be exposed to applications, so we don't install
 * this header file
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef _CalibrationProcess_h
#define _CalibrationProcess_h

#include <AstroGuiding.h>
#include <GuidingProcess.h>
#include <pthread.h>

using namespace astro::camera;
using namespace astro::image;

namespace astro {
namespace guiding {

/**
 * \brief Encapsulation of the guiding process
 *
 * This class contains the work function for guider calibration.
 */
class CalibrationProcess : public GuidingProcess {

	// parameters for the calibration process
	/**
	 * \brief focal length of guide scope in mm
	 */
	double	_focallength;
	/**
	 * \brief Pixel size in um
 	 */
	double	_pixelsize;
	double	grid;
	bool	calibrated;
	double	_progress;
	int	range;
	double	currentprogress(int ra, int dec) const;
public:
	double	progress() const { return _progress; }
private:
	double	gridconstant(double focallength, double pixelsize) const;
	Point	pointat(double ra, double dec);
	void	moveto(double ra, double dec);
	void	measure(GuiderCalibrator& calibrator,
			double deltara, double deltadec);

public:
	CalibrationProcess(Guider& guider, TrackerPtr tracker);
	~CalibrationProcess();
	void	calibrate(double focallength, double pixelsize);
	// the main function of the process
	void	main(GuidingThread<CalibrationProcess>& thread);
};

} // namespace guiding
} // namespace astro

#endif /* _CalibrationProcess_h */