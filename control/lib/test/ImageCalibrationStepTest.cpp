/*
 * ImageCalibrationStepTest.cpp -- template for tests
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <cppunit/TestFixture.h>
#include <cppunit/TestAssert.h>
#include <cppunit/extensions/HelperMacros.h>
#include <AstroDebug.h>
#include <AstroProcess.h>
#include <AstroImage.h>

using namespace astro::image;
using namespace astro::process;

namespace astro {
namespace test {

class ImageCalibrationStepTest: public CppUnit::TestFixture {
public:
	void	setUp();
	void	tearDown();
	void	testCalibration();

	CPPUNIT_TEST_SUITE(ImageCalibrationStepTest);
	CPPUNIT_TEST(testCalibration);
	CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ImageCalibrationStepTest);

void	ImageCalibrationStepTest::setUp() {
}

void	ImageCalibrationStepTest::tearDown() {
}

void	ImageCalibrationStepTest::testCalibration() {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "testCalibration() begin");
	// create calibration images
	ImageSize	size(40, 32);
	Image<float>	*dark = new Image<float>(size);
	ImagePtr	darkptr(dark);
	Image<float>	*flat = new Image<float>(size);
	ImagePtr	flatptr(flat);
	Image<double>	*image = new Image<double>(size);
	ImagePtr	imageptr(image);
	for (unsigned int x = 0; x < size.width(); x++) {
		for (unsigned int y = 0; y < size.height(); y++) {
			//debug(LOG_DEBUG, DEBUG_LOG, 0, "%u,%u", x, y);
			double	v = 32768;
			double	offset = 1024 + random() % 128;
			dark->writablepixel(x, y) = offset;
			float	d = size.center().distance(ImagePoint(x, y));
			double	f = 1 + 0.001 * d;
			flat->writablepixel(x, y) = 1. / f;
			double	raw = v / f + offset;
			image->writablepixel(x, y) = raw;
			//debug(LOG_DEBUG, DEBUG_LOG, 0,
			//	"(%u,%u), dark=%f, flat=%f, raw=%f", x, y,
			//	offset, f, raw);
		}
	}
	debug(LOG_DEBUG, DEBUG_LOG, 0, "images prepared");

	// prepare a controll that will do the work
	ProcessingController	controller;

	// create calibration image steps
	CalibrationImageStep	*darkstep
		= new CalibrationImageStep(CalibrationImageStep::DARK, darkptr);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "dark: %p", darkstep);
	ProcessingStepPtr	darkstepptr(darkstep);
	CPPUNIT_ASSERT(darkstepptr->status() == ProcessingStep::needswork);
	controller.addstep("dark", darkstepptr);

	CalibrationImageStep	*flatstep
		= new CalibrationImageStep(CalibrationImageStep::FLAT, flatptr);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "flat: %p", flatstep);
	ProcessingStepPtr	flatstepptr(flatstep);
	CPPUNIT_ASSERT(flatstepptr->status() == ProcessingStep::needswork);
	controller.addstep("flat", flatstepptr);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "calibration image step created");

	// create uncalibrated image step
	RawImageStep	*rawstep = new RawImageStep(imageptr);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "raw: %p", rawstep);
	ProcessingStepPtr	rawstepptr(rawstep);
	CPPUNIT_ASSERT(rawstepptr->status() == ProcessingStep::needswork);
	controller.addstep("raw", rawstepptr);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "raw image step created");

	// build the calibration step
	ImageCalibrationStep	*calibrationstep = new ImageCalibrationStep();
	debug(LOG_DEBUG, DEBUG_LOG, 0, "calibration: %p", calibrationstep);
	ProcessingStepPtr	calibrationstepptr(calibrationstep);
	controller.addstep("calibration", calibrationstepptr);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "image calibration step created");

	// define dependencies
	controller.add_precursor("calibration", "dark");
	controller.add_precursor("calibration", "flat");
	controller.add_precursor("calibration", "raw");
	debug(LOG_DEBUG, DEBUG_LOG, 0, "precursors set");

#if 0
	darkstep->work();
	CPPUNIT_ASSERT(darkstep->status() == ProcessingStep::complete);
	flatstep->work();
	CPPUNIT_ASSERT(flatstep->status() == ProcessingStep::complete);
	rawstep->work();
	CPPUNIT_ASSERT(rawstep->status() == ProcessingStep::complete);

	calibrationstep->work();
	CPPUNIT_ASSERT(calibrationstep->status() == ProcessingStep::complete);
	
#else
	// perform the calibration
	debug(LOG_DEBUG, DEBUG_LOG, 0, "start process execution");
	controller.execute(2);
#endif
	CPPUNIT_ASSERT(calibrationstepptr->status()
			== ProcessingStep::complete);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "state after execution: %s",
		ProcessingStep::statename(calibrationstepptr->status()).c_str());

	// verify that calibration returns the correct value
	for (unsigned int x = 0; x < size.width(); x++) {
		for (unsigned int y = 0; y < size.height(); y++) {
			double	rawv = image->pixel(x, y);
			double	calibratedv = calibrationstep->out().pixel(x, y);
			//debug(LOG_DEBUG, DEBUG_LOG, 0,
			//	"%u,%u: rawv = %f, v = %f", x, y, rawv,
			//	calibratedv);
			CPPUNIT_ASSERT(round(calibratedv) == 32768);
		}
	}

	// that's it
	debug(LOG_DEBUG, DEBUG_LOG, 0, "testCalibration() end");
}

} // namespace test
} // namespace astro
