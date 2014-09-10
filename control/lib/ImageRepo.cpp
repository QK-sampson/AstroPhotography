/*
 * ImageRepo.cpp -- implementation of the image repository
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <AstroProject.h>
#include <AstroDebug.h>
#include <AstroIO.h>
#include <includes.h>
#include <ImageRepoTables.h>
#include <numeric>

using namespace astro::persistence;
using namespace astro::image;
using namespace astro::io;
using namespace astro::camera;

namespace astro {
namespace project {

/**
 * \brief Create an image server
 */
ImageRepo::ImageRepo(Database database, const std::string& directory,
	bool scan)
	: _database(database), _directory(directory) {
	// scan the directory for 
	if (scan) {
		scan_directory();
	}
}

/**
 * \brief get the id of an image identified by its filename
 */
long	ImageRepo::id(const std::string& filename) {
	ImageTable	images(_database);
	return images.id(filename);
}

/**
 * \brief process a single file during a scan
 */
void	ImageRepo::scan_file(const std::string& filename) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "scanning file '%s' (%d)",
		filename.c_str(), filename.length());
	// does the filename end in ".fits"?
	if (filename.size() < 5) {
		return;
	}
	std::string	extension = filename.substr(filename.length() - 5, 5);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "substr: '%s'", extension.c_str());
	if (extension != ".fits") {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "%s not a FITS file",
			filename.c_str());
		return;
	}

	// check whether this is a file
	std::string	fullname = _directory + "/" + filename;
	struct stat	sb;
	if (stat(fullname.c_str(), &sb) < 0) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "cannot stat file %s: %s",
			fullname.c_str(), strerror(errno));
		return;
	}

	if (!S_ISREG(sb.st_mode)) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "%s: not a regular file",
			fullname.c_str());
		return;
	}

	// find out whether the database already contains this filename
	ImageTable	images(_database);
	try {
		long	id = images.id(filename);
		debug(LOG_DEBUG, DEBUG_LOG, 0, "%s exists with id %ld", 
			filename.c_str(), id);
		// if we get to this point, the file already exists, so we
		// skip it
		return;
	} catch (...) {
		// file does not exist
	}

	// read the metadata
        io::FITSinfileBase      infile(fullname);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "size: %s",
		infile.getSize().toString().c_str());

	// create an information record
	ImageRecord	imageinfo;
	imageinfo.filename = filename;
	imageinfo.project = "unknown";
	imageinfo.created = sb.st_ctime;
	try {
		imageinfo.camera
			= (std::string)infile.getMetadata("INSTRUME");
	} catch(...) { }
	imageinfo.width = infile.getSize().width();
	imageinfo.height = infile.getSize().height();
	imageinfo.xbin = 1;
	try {
		imageinfo.xbin
			= (int)infile.getMetadata("XBINNING");
	} catch(...) { }
	imageinfo.ybin = 1;
	try {
		imageinfo.ybin
			= (int)infile.getMetadata("YBINNING");
	} catch(...) { }
	imageinfo.depth = infile.getPlanes();
	imageinfo.pixeltype = infile.getPixeltype();
	imageinfo.exposuretime = 0;
	try {
		imageinfo.exposuretime
			= (double)infile.getMetadata("EXPTIME");
	} catch(...) { }
	imageinfo.temperature = 0;
	try {
		imageinfo.temperature
			= (double)infile.getMetadata("CCD-TEMP");
	} catch(...) { }
	imageinfo.category = "light";
	imageinfo.bayer = "    ";
	imageinfo.observation = "1970-01-01T00:00:00.000";
	imageinfo.uuid = "";
	try {
		imageinfo.uuid = (std::string)(infile.getMetadata("UUID"));
	} catch (...) { }

	// add the entry to the table
	long	imageid = images.add(imageinfo);

	// in the part below we need the metatdata table
	MetadataTable	metadatatable(_database);

	// get all the metadata from the infile
	int	seqno = 0;
	ImageMetadata	md = infile.getAllMetadata();

	// now add an entry for each meta data record
	ImageMetadata::const_iterator	mi;
	for (mi = md.begin(); mi != md.end(); mi++) {
		MetadataRecord	m(-1, imageid);
		m.seqno = seqno;
		m.key = mi->first;
		m.value = mi->second.getValue();
		m.comment = mi->second.getComment();
		metadatatable.add(m);
		seqno++;
	}
	debug(LOG_DEBUG, DEBUG_LOG, 0, "%d metadata records added", seqno);
}

/**
 * \brief Scan a directory for images
 */
void	ImageRepo::scan_directory(bool recurse) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "scan directory %s", _directory.c_str());

	if (recurse) {
		debug(LOG_DEBUG, DEBUG_LOG, 0,
			"recursive scan not implemented");
		throw std::runtime_error("not implemented");
		return;
	}

	// set up a counter so we can count the number of files we find
	int	counter = 0;

	// open the directory
	DIR     *dir = opendir(_directory.c_str());
	if (NULL == dir) {
		throw std::runtime_error("cannot open directory");
	}
	struct dirent	*d;
	while (NULL != (d = readdir(dir))) {
		std::string     filename(d->d_name);
		scan_file(filename);
		counter++;
	}

	// close the directory
	closedir(dir);

	// report the number of files scanned
	debug(LOG_DEBUG, DEBUG_LOG, 0, "%d files scanned", counter);
}

/**
 * \brief Retrieve an image
 */
std::string	ImageRepo::filename(long id) {
	return ImageTable(_database).byid(id).filename;
}

std::string	ImageRepo::pathname(long id) {
	return _directory + "/" + filename(id);
}

/**
 * \brief Get an image
 */
ImagePtr	ImageRepo::getImage(long id) {
	std::string	f = pathname(id);
	FITSin	in(f);
	return in.read();
}

static ImageEnvelope	convert(const ImageRecord& imageinfo,
				MetadataTable& metadatatable) {
	ImageEnvelope	result(imageinfo.id());

	// image geometry
	result.size(ImageSize(imageinfo.width, imageinfo.width));
	result.binning(Binning(imageinfo.xbin, imageinfo.ybin));

	// retrieve all the metadata available
	std::string	condition = stringprintf("imageid = %ld",
					imageinfo.id());
	std::list<MetadataRecord>	mdrecords
		= metadatatable.select(condition);

	// convert the MetadataRecords into actual metadata
	std::list<MetadataRecord>::const_iterator	mi;
	for (mi = mdrecords.begin(); mi != mdrecords.end(); mi++) {
		Metavalue	m = FITSKeywords::meta(mi->key, mi->value,
					mi->comment);
		result.metadata.setMetadata(m);
	}

	// envelope variables
	result.filename(imageinfo.filename);
	result.project(imageinfo.project);
	result.created(imageinfo.created);
	result.camera(imageinfo.camera);
	result.exposuretime(imageinfo.exposuretime);
	result.temperature(imageinfo.temperature);
	if (imageinfo.category == "light") {
		result.category(ImageSpec::light);
	}
	if (imageinfo.category == "dark") {
		result.category(ImageSpec::dark);
	}
	if (imageinfo.category == "flat") {
		result.category(ImageSpec::flat);
	}
	result.bayer(imageinfo.bayer);
	result.observation((time_t)FITSdate(imageinfo.observation));
	result.uuid(UUID(imageinfo.uuid));

	// we are done, return the envelope
	return result;
}

/**
 * \brief Retrieve the metadata for an image
 */
ImageEnvelope	ImageRepo::getEnvelope(long id) {
	// create a result record
	ImageEnvelope	result(id);

	// read the global information from the database
	ImageRecord	imageinfo = ImageTable(_database).byid(id);
	MetadataTable	metadatatable(_database);
	return convert(imageinfo, metadatatable);
}

/**
 * \brief Save an image in the repository
 */
long	ImageRepo::save(ImagePtr image) {
	// first we have to create a file name for the image
	char	buffer[MAXPATHLEN];
	snprintf(buffer, sizeof(buffer), "%s/image-XXXXX.fits",
		_directory.c_str());
	if (mkstemps(buffer, 5) < 0) {
		std::string	msg
			= stringprintf("cannot create a filename: %s",
				strerror(errno));;
		debug(LOG_DEBUG, DEBUG_LOG, 0, "%s", msg.c_str());
		throw std::runtime_error(msg);
	}

	std::string	fullname = std::string(buffer);
	std::string	filename = fullname.substr(_directory.size() + 1);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "full name: %s", fullname.c_str());

	// if the image does not have a UUID yet, add one
	if (!image->hasMetadata("UUID")) {
		image->setMetadata(FITSKeywords::meta("UUID",
			(std::string)UUID()));
	}

	// write the image
	unlink(fullname.c_str());
	FITSout	out(fullname);
	out.write(image);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "image written to %s", fullname.c_str());

	// build imageinfo reocord
	ImageRecord	imageinfo(-1);
	imageinfo.filename = filename;
	try {
		imageinfo.project
			= (std::string)image->getMetadata("PROJECT");
	} catch (...) { }
	try {
		imageinfo.camera
			= (std::string)image->getMetadata("INSTRUME");
	} catch (...) { }
	imageinfo.width = image->size().width();
	imageinfo.height = image->size().height();
	try {
		imageinfo.xbin
			= (int)image->getMetadata("XBINNING");
	} catch (...) { }
	try {
		imageinfo.ybin
			= (int)image->getMetadata("YBINNING");
	} catch (...) { }
	imageinfo.depth = image->planes();
	imageinfo.pixeltype = image->bitsPerPlane();
	try {
		imageinfo.exposuretime
			= (double)image->getMetadata("EXPTIME");
	} catch (...) { }
	try {
		imageinfo.temperature
			= (double)image->getMetadata("CCD-TEMP");
	} catch (...) { }
	try {
		imageinfo.category
			= (std::string)image->getMetadata("PURPOSE");
	} catch (...) { }
	try {
		imageinfo.bayer
			= (std::string)image->getMetadata("BAYER");
	} catch (...) { }
	try {
		imageinfo.observation
			= (std::string)image->getMetadata("DATE-OBS");
	} catch (...) { }
	try {
		imageinfo.uuid
			= (std::string)image->getMetadata("UUID");
	} catch (...) { }

	// save the image info
	ImageTable	images(_database);
	long	imageid = images.add(imageinfo);

	// write the metadata to the metadata tabe
	MetadataTable	metadata(_database);
	
	// now add an entry for each meta data record
	ImageMetadata::const_iterator	mi;
	long	seqno = 0;
	for (mi = image->begin(); mi != image->end(); mi++) {
		MetadataRecord	m(-1, imageid);
		m.seqno = seqno;
		m.key = mi->first;
		m.value = mi->second.getValue();
		m.comment = mi->second.getComment();
		metadata.add(m);
		seqno++;
	}
	debug(LOG_DEBUG, DEBUG_LOG, 0, "%d metadata records added", seqno);

	// done
	return imageid;
}

/**
 * \brief Remove the image and the metadata from the database
 */
void	ImageRepo::remove(long id) {
	ImageTable(_database).remove(id);
}

static float	temperature_min(float temperature) {
	return 0.99 * (273.15 + temperature) - 273.15;
}

static float	temperature_max(float temperature) {
	return 1.01 * (273.15 + temperature) - 273.15;
}

class condition : public std::string {
public:
	condition(const std::string& s) : std::string(s) { }

	condition	operator+(const condition& other) {
		if ((size() == 0) && (other.size() == 0)) {
			return std::string("");
		}
		if (size() == 0) {
			return other;
		}
		if (other.size() == 0) {
			return *this;
		}
		return condition("(" + *this + ") and (" + other + ")");
	}
};

/**
 * \brief get a set of images matching the specifcation
 */
std::set<ImageEnvelope>	ImageRepo::get(const ImageSpec& spec) {
	std::list<condition>	conditions;
	// add category condition
	switch (spec.category()) {
	case ImageSpec::light:
		conditions.push_back(condition("category = 'light'"));
		break;
	case ImageSpec::dark:
		conditions.push_back(condition("category = 'dark'"));
		break;
	case ImageSpec::flat:
		conditions.push_back(condition("category = 'flat'"));
		break;
	}

	// add cameraname condition
	if (spec.camera().size() > 0) {
		conditions.push_back(condition(stringprintf("cameraname = '%s'",
			spec.camera().c_str())));
	}

	// add exposure time condition
	if (spec.exposuretime() > 0) {
		conditions.push_back(condition(stringprintf(
			"%f <= exposuretime and exposuretime <= %f",
			spec.exposuretime() * 0.9, spec.exposuretime() * 1.1)));
	}

	// add temperature condition
	if (spec.temperature() > -273.15) {
		conditions.push_back(condition(stringprintf(
			"%f <= temperature and temperature < %f",
			temperature_min(spec.temperature()),
			temperature_max(spec.temperature()))));
	}

	// add project condition
	if (spec.project().size() > 0) {
		conditions.push_back(condition(stringprintf(
			"project = '%s'", spec.project().c_str())));
	}

	// concatenate the conditions
	condition	all =  std::accumulate(conditions.begin(),
					conditions.end(), condition(""));
	debug(LOG_DEBUG, DEBUG_LOG, 0, "accumulated condition %s", all.c_str());

	// query the database 
	ImageTable	imagetable(_database);
	MetadataTable	metadatatable(_database);

	std::list<ImageRecord>	images = imagetable.select(all);

	// build the result set
	std::set<ImageEnvelope>	resultset;
	std::list<ImageRecord>::const_iterator	ii;
	for (ii = images.begin(); ii != images.end(); ii++) {
		resultset.insert(convert(*ii, metadatatable));
	}
	return resultset;
}

} // namespace project
} // namespace astro
