/*
 * MappedFile.h -- Mixin class for memory mapped files for various
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef _MappedFile_h
#define _MappedFile_h

#include <string>

namespace astro {
namespace catalog {

/**
 * \brief Abstraction for files mapped into the address space
 */
class MappedFile {
	size_t	_recordlength;
	void	*data_ptr;
	size_t	data_len;
	int	_nrecords;
public:
	int	nrecords() const { return _nrecords; }
private:
	// private copy constructors to prevent copying
	MappedFile(const MappedFile&);
	MappedFile&	operator=(const MappedFile&);
public:
	MappedFile(const std::string& filename, size_t recordlength);
	~MappedFile();
	std::string	get(size_t record_number) const;
};

} // namespace catalog
} // namespace astro

#endif /* _MappedFile_h */
