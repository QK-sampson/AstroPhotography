/*
 * Fits.cpp -- implementation of FITS io routines
 *
 * (c) 2012 Prof Dr Andreas Mueller, Hochschule Rapperswil
 * $Id$
 */
#include <AstroIO.h>
#include <fitsio.h>
#include <debug.h>

using namespace astro::image;

namespace astro {
namespace io {

/**
 * \brief Retrieve a human readable error message from the fits library
 */
std::string	FITSfile::errormsg(int status) const {
	char	errmsg[128];
	fits_get_errstatus(status, errmsg);
	return std::string(errmsg);
}

/**
 * \brief Construct a FITS file object
 *
 * This does not open a file, which is reserved to the derived classes.
 */
FITSfile::FITSfile(const std::string& _filename,
	int _pixeltype, int _planes, int _imgtype)
	: filename(_filename), fptr(NULL), pixeltype(_pixeltype),
	  planes(_planes), imgtype(_imgtype) {
}

/**
 * \brief Destroy a FITS file.
 *
 * This destructor closes the file, if it is open
 */
FITSfile::~FITSfile() throw (FITSexception) {
	if (NULL == fptr) {
		return;
	}
	int	status = 0;
	if (fits_close_file(fptr, &status)) {
		throw FITSexception(errormsg(status));
	}
	fptr = NULL;
}

/**
 * \brief Open a FITS file for reading
 */
FITSinfileBase::FITSinfileBase(const std::string& filename) throw (FITSexception)
	: FITSfile(filename, 0, 0, 0) {
	int	status = 0;
	if (fits_open_file(&fptr, filename.c_str(), READONLY, &status)) {
		throw FITSexception(errormsg(status));
	}

	/* read the dimensions of the image from the file */
	int	naxis;
	long	naxes[3];
	int	igt;
	if (fits_get_img_param(fptr, 3, &igt, &naxis, naxes, &status)) {
		throw FITSexception(errormsg(status));
	}
	imgtype = igt;
	debug(LOG_DEBUG, DEBUG_LOG, 0, "params read: imgtype = %d", imgtype);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "             naxis = %d", naxis);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "             naxes[] = [%d,%d,%d]",
		naxes[0], naxes[1], naxes[2]);
	switch (naxis) {
	case 2:
		planes = 1;
		break;
	case 3:
		planes = naxes[2];
		break;
	default:
		throw FITSexception("don't know what to do with image of dimension != 2 or 3");
	}
	size = ImageSize(naxes[0], naxes[1]);
	
	switch (planes) {
	case 1:
	case 3:
		break;
	default:
		throw std::runtime_error("not 1 or 3 planes");
	}

	// now read the keys
	readkeys();
}

/**
 * \brief Read the raw data
 */
void	*FITSinfileBase::readdata() throw (FITSexception) {
	int	typesize = 0;
	debug(LOG_DEBUG, DEBUG_LOG, 0, "reading an image with image type %d",
		imgtype);
	switch (imgtype) {
	case BYTE_IMG:
	case SBYTE_IMG:
			typesize = sizeof(char);
			pixeltype = TBYTE;
			break;
	case USHORT_IMG:
	case SHORT_IMG:
			typesize = sizeof(short);
			pixeltype = TUSHORT;
			break;
	case ULONG_IMG:
	case LONG_IMG:
			typesize = sizeof(long);
			pixeltype = TULONG;
			break;
	case FLOAT_IMG:
			typesize = sizeof(float);
			pixeltype = TFLOAT;
			break;
	case DOUBLE_IMG:
			typesize = sizeof(double);
			pixeltype = TDOUBLE;
			break;
	default:
		debug(LOG_ERR, DEBUG_LOG, 0, "unknown pixel type %d", imgtype);
		throw FITSexception("cannot read this pixel type");
		break;
	}
	void	*v = calloc(planes * size.pixels, typesize);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "data size: %d items of size %d, "
		"pixel type %d, %d planes", (size.pixels * planes), typesize,
		pixeltype, planes);
	
	/* now read the data */
	int	status = 0;
	long	firstpixel[3] = { 1, 1, 1 };
	if (fits_read_pix(fptr, pixeltype, firstpixel, size.pixels * planes,
		NULL, v, NULL, &status)) {
		free(v);
		throw FITSexception(errormsg(status));
	}
	debug(LOG_DEBUG, DEBUG_LOG, 0, "fits data read");
	return v;
}

#define	IGNORED_KEYWORDS_N	8
const char	*ignored_keywords[IGNORED_KEYWORDS_N] = {
	"SIMPLE", "BITPIX", "PCOUNT", "GCOUNT",
	"XTENSION", "END", "BSCALE", "BZERO"
};

static bool	ignored(const std::string& keyname) {
	if (keyname.substr(0, 5) == "NAXIS") {
		return true;
	}
	for (int i = 0; i < IGNORED_KEYWORDS_N; i++) {
		if (keyname == ignored_keywords[i]) {
			return true;
		}
	}
	return false;
}

/**
 * \brief Read the headers from a FITS file
 *
 * In the headers we only record the headers that are not managed by
 * the type stuff. I.e. the keywords SIMPLE, BITPIX, NAXIS, NAXISn, END,
 * PCOUNT, GCOUNT, XTENSION are ignored
 */
void	FITSinfileBase::readkeys() throw (FITSexception) {
	int	status = 0;
	int	keynum = 1;
	char	keyname[100];
	char	value[100];
	char	comment[100];
	while (1) {
		if (fits_read_keyn(fptr, keynum, keyname, value, comment,
			&status)) {
			// we are at the end of the headers, so we return
			debug(LOG_DEBUG, DEBUG_LOG, 0, "%d headers read",
				keynum);
			return;
		}
		FITShdu	hdu;		
		hdu.name = keyname;
		if (!ignored(hdu.name)) {
			hdu.value = value;
			hdu.comment = comment;
			headers.insert(make_pair(hdu.name, hdu));
			debug(LOG_DEBUG, DEBUG_LOG, 0, "%s = %s/%s",
				hdu.name.c_str(), hdu.value.c_str(),
				hdu.comment.c_str());
		}
		keynum++;
	}
}

/**
 * \brief Create a FITS file for writing
 */
FITSoutfileBase::FITSoutfileBase(const std::string &filename,
	int pixeltype, int planes, int imgtype) throw (FITSexception)
	: FITSfile(filename, pixeltype, planes, imgtype)  {
	int	status = 0;
	if (fits_create_file(&fptr, filename.c_str(), &status)) {
		throw FITSexception(errormsg(status));
	}
}

/**
 * \brief write the image format information to the header
 */
void	FITSoutfileBase::write(const ImageBase& image) throw (FITSexception) {
	// find the dimensions
	long	naxis = 3;
	long	naxes[3] = { image.size.width, image.size.height, planes };
	int	status = 0;
	if (fits_create_img(fptr, imgtype, naxis, naxes, &status)) {
		throw FITSexception(errormsg(status));
	}
}

/**
 * \brief constructor specializations of FITSoutfile for all types
 */
#define FITS_OUT_CONSTRUCTOR(T, pix, planes, img)			\
template<>								\
FITSoutfile<T >::FITSoutfile(const std::string& filename)		\
	throw (FITSexception)						\
	: FITSoutfileBase(filename, pix, planes, img) {			\
}

// basic type monochrome pixels
FITS_OUT_CONSTRUCTOR(unsigned char, TBYTE, 1, BYTE_IMG)
FITS_OUT_CONSTRUCTOR(unsigned short, TUSHORT, 1, USHORT_IMG)
FITS_OUT_CONSTRUCTOR(unsigned int, TULONG, 1, ULONG_IMG)
FITS_OUT_CONSTRUCTOR(unsigned long, TULONG, 1, ULONG_IMG)
FITS_OUT_CONSTRUCTOR(float, TFLOAT, 1, FLOAT_IMG)
FITS_OUT_CONSTRUCTOR(double, TDOUBLE, 1, DOUBLE_IMG)

// RGB Pixels
FITS_OUT_CONSTRUCTOR(RGB<unsigned char> , TBYTE, 3, BYTE_IMG)
FITS_OUT_CONSTRUCTOR(RGB<unsigned short> , TUSHORT, 3, USHORT_IMG)
FITS_OUT_CONSTRUCTOR(RGB<unsigned int> , TUINT, 3, ULONG_IMG)
FITS_OUT_CONSTRUCTOR(RGB<unsigned long> , TULONG, 3, ULONG_IMG)
FITS_OUT_CONSTRUCTOR(RGB<float> , TFLOAT, 3, FLOAT_IMG)
FITS_OUT_CONSTRUCTOR(RGB<double> , TDOUBLE, 3, DOUBLE_IMG)

// YUYV Pixels
FITS_OUT_CONSTRUCTOR(YUYV<unsigned char>, TBYTE, 3, BYTE_IMG)
FITS_OUT_CONSTRUCTOR(YUYV<unsigned short>, TUSHORT, 3, USHORT_IMG)
FITS_OUT_CONSTRUCTOR(YUYV<unsigned int>, TULONG, 3, ULONG_IMG)
FITS_OUT_CONSTRUCTOR(YUYV<unsigned long>, TULONG, 3, ULONG_IMG)
FITS_OUT_CONSTRUCTOR(YUYV<float>, TFLOAT, 3, FLOAT_IMG)
FITS_OUT_CONSTRUCTOR(YUYV<double>, TDOUBLE, 3, DOUBLE_IMG)

} // namespace io
} // namespace astro

