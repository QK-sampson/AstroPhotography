/*
 * AstroPersistence.h -- Some utility classes for persistence
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef _AstroPersistence_h
#define _AstroPersistence_h

#include <string>
#include <list>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <sys/time.h>

namespace astro {

/**
 * \brief A simple persistence layer for the astrophotography project
 *
 * Metadata for astrophotography projects as well as taks information 
 * for the task manager need to be stored in an sqlite3 database. 
 * This namespace contain some classes an templates that simplify 
 * database access.
 */
namespace persistence {

/**
 * \brief Exception thrown when no rows are found
 */
class NotFound : public std::runtime_error {
public:
	NotFound(const std::string& cause) : std::runtime_error(cause) { }
	NotFound(const char *cause) : std::runtime_error(cause) { }
};

/**
 * \brief Exception thrown when there is no usable database
 */
class BadDatabase : public std::runtime_error {
public:
	BadDatabase(const std::string& cause) : std::runtime_error(cause) { }
	BadDatabase(const char *cause) : std::runtime_error(cause) { }
};

/**
 * \brief Exception thrown when there is a problem with the SQL syntax
 */
class BadQuery : public std::runtime_error {
public:
	BadQuery(const std::string& cause) : std::runtime_error(cause) { }
	BadQuery(const char *cause) : std::runtime_error(cause) { }
};

/**
 * \brief abstraction field field values
 */
class FieldValue {
public:
	virtual int	intValue() const = 0;
	virtual double	doubleValue() const = 0;
	virtual std::string	stringValue() const = 0;
	virtual bool	isnull() const { return false; }
	virtual std::string	toString() const { return stringValue(); }
	virtual time_t	timeValue() const = 0;
	virtual struct timeval	timevalValue() const = 0;
};

typedef std::shared_ptr<FieldValue>	FieldValuePtr;

/**
 * \brief Factory class to produce field value objects
 */
class FieldValueFactory {
public:
	FieldValuePtr	get(int value) const;
	FieldValuePtr	get(double value) const;
	FieldValuePtr	get(const std::string& value) const;
	FieldValuePtr	get(const char *value) const;
	FieldValuePtr	getTime(const time_t t) const;
	FieldValuePtr	getTime(const std::string& value) const;
	FieldValuePtr	getTimeval(const struct timeval& value) const;
	FieldValuePtr	getTimeval(const std::string& value) const;
	FieldValuePtr	getTimeval(double value) const;
};

/**
 * \brief Wrapper class for field values
 */
class Field : public std::pair<std::string, FieldValuePtr> {
public:
	Field(const std::string& name, FieldValuePtr value)
		: std::pair<std::string, FieldValuePtr>(name, value) { }
	int	intValue() const { return second->intValue(); }
	double	doubleValue() const { return second->doubleValue(); }
	std::string	stringValue() const { return second->stringValue(); }
	time_t	timeValue() const { return second->timeValue(); }
	struct timeval	timevalValue() const { return second->timevalValue(); }
	bool	operator==(const std::string& othername) const {
		return first == othername;
	}
	const std::string&	name() const { return first; }
	const FieldValuePtr	value() const { return second; }
};

std::ostream&	operator<<(std::ostream& out, const Field& field);

/**
 * \brief A Row is a vector of fields, together with a vector of field names
 * 
 * A row can access fiel values by name
 */
class Row : public std::vector<Field> {
public:
	const FieldValuePtr&	operator[](const size_t idx) const {
		return std::vector<Field>::operator[](idx).second;
	}
	const FieldValuePtr&	operator[](const std::string& fieldname) const {
		std::vector<Field>::const_iterator	i
			= std::find(begin(), end(), fieldname);
		if (i == end()) {
			throw NotFound("column name not found");
		}
		return i->second;
	}
};

std::ostream&	operator<<(std::ostream& out, const Row& row);

/**
 * \brief A query result is a list of rows, and a list of field names
 */
class Result : public std::list<Row> {
public:
};

std::ostream&	operator<<(std::ostream& out, const Result& result);

/**
 * \brief Interface for statements
 *
 * This essentially encapsulates a query string with place holders.
 * The sqlite3 database provides many methods to bind values to those
 * place holders, this class provides a strictly typed object oriented
 * interface to the same functionality, and eases the transition to the
 * types used in the rest of the system.
 */
class Statement {
	std::string	_query;
public:
	const std::string&	query() const { return _query; }
	Statement(const std::string& query) : _query(query) { }
	virtual void	bindInteger(int colno, int value) = 0;
	void	bind(int colno, int value) {
		bindInteger(colno, value);
	}
	virtual void	bindDouble(int colno, double value) = 0;
	void	bind(int colno, double value) {
		bindDouble(colno, value);
	}
	virtual void	bindString(int colno, const std::string& value) = 0;
	void	bind(int colno, const std::string& value) {
		bindString(colno, value);
	}
	void	bind(int colno, const FieldValuePtr& value);
	virtual void execute() = 0;
protected:
	virtual Field	field(int colno) = 0;
	virtual Row	row() = 0;
public:
	virtual Result	result() = 0;
	virtual int	integerColumn(int colno) = 0;
	virtual double	doubleColumn(int colno) = 0;
	virtual std::string	stringColumn(int colno) = 0;
};

typedef std::shared_ptr<Statement>	StatementPtr;

/**
 * \brief The generic backend interface
 *
 * This class only defines the interface, a derived backend class will
 * implement the methods for a particular type of database. Thus applications
 * using this interface are not tied to the actual database system used.
 */
class DatabaseBackend {
public:
	virtual std::string	escape(const std::string& value) = 0;
	virtual Result	query(const std::string& query) = 0;
	virtual std::vector<std::string>
		fieldnames(const std::string& tablename) = 0;
	virtual void	begin() = 0;
	virtual void	begin(const std::string& savepoint) = 0;
	virtual void	commit() = 0;
	virtual void	commit(const std::string& savepoint) = 0;
	virtual void	rollback() = 0;
	virtual void	rollback(const std::string& savepoint) = 0;
	virtual StatementPtr	statement(const std::string& query) = 0;
	virtual bool	hastable(const std::string& tablename) = 0;
};
typedef std::shared_ptr<DatabaseBackend>	Database;

/**
 * \brief A factory for creating backends
 */
class DatabaseFactory {
public:
static Database	get(const std::string& name);
};

/**
 * \brief Update Specification used for the database independent interface
 */
class UpdateSpec : public std::map<std::string, FieldValuePtr> {
public:
	std::string	columnlist() const;
	std::string	values() const;
	std::string	update() const;
public:
	std::string	selectquery(const std::string& tablename) const;
	std::string	insertquery(const std::string& tablename) const;
	std::string	updatequery(const std::string& tablename) const;
	void	bind(StatementPtr& stmt) const;
	void	bindid(StatementPtr& stmt, int id) const;
};

/**
 * \brief Table objects are a very stripped down OR mapper
 */
class TableBase {
protected:
	Database	_database;
	std::string	_tablename;
	std::vector<std::string>	_fieldnames;
	std::string	selectquery() const;
protected:
	Database	database() { return _database; }
public:
	TableBase(Database database, const std::string& tablename,
		const std::string& createstatement = std::string());
	Row	rowbyid(long objectid);
	long	lastid();
	long	nextid();
	long	addrow(const UpdateSpec& updatespec);
	virtual long	id(const std::string& condition);
	long	count();
	long	count(const std::string& condition);
	void	updaterow(long objectid, const UpdateSpec& updatespec);
	bool	exists(long objectid);
	void	remove(long objectid);
	void	remove(const std::list<long>& objectids);
	void	remove(const std::string& condition);
	std::list<long>	selectids(const std::string& condition);
	Result	selectrows(const std::string& condition);
	bool	has(const std::string& condition);
};

/**
 * \brief Template to create a persistent version of an object
 *
 * A persistent version of an object has an id field that allows to identify
 * the corresponding record in the database.
 */
template<typename object>
class Persistent : public object {
	int	_id;
public:
	const int&	id() const { return _id; }
	void	id(const int& i) { _id = i; }
	Persistent(int i) : _id(i) { }
	Persistent(int i, const object& _object) : object(_object), _id(i) { }
	Persistent(const object& _object, int i = -1)
		: object(_object), _id(i) { }
};

template<typename target, typename source>
std::list<target>	ObjectList(const std::list<source>& l) {
	std::list<target>	result;
	std::copy(l.begin(), l.end(), back_inserter(result));
	return result;
}

/**
 * \brief Template to create a persistent version of an object with reference
 *
 * This is a persistent version, i.e. it has an id, but it also has a reference
 * to another persistent object.
 */
template<typename object>
class PersistentRef : public Persistent<object> {
	int	_ref;
public:
	const int&	ref() const { return _ref; }
	void	ref(const int& r) { _ref = r; }
	PersistentRef(int i, int r)
		: Persistent<object>(i), _ref(r) { }
	PersistentRef(int i, int r, const object& _object)
		: Persistent<object>(i, _object), _ref(r) { }
};

// The table template create below from the TableBase class needs a
// Table descriptor class that needs some
//
// class table_adapter {
// static std::string	tablename();
// static std::string	createstatement();
// object	row_to_object(int objectid, const Row& row);
// UpdateSpec	object_to_updatespec(const object& o);
// };
//
// These methods are needed by the Table template to build the table entries

/**
 * \brief Template to map any type of object to a database table
 */
template<typename object, typename dbadapter>
class Table : public TableBase {
public:
	typedef	object object_type;
	Table(Database database)
		: TableBase(database, dbadapter::tablename(),
			dbadapter::createstatement()) { }
	object	byid(long objectid);
	long	add(const object&);
	void	update(long objectid, const object& o);
	std::list<object>	select(const std::string& condition) {
		std::list<object>	result;
		Result	rows = selectrows(condition);
		Result::const_iterator	i;
		for (i = rows.begin(); i != rows.end(); i++) {
			int	id = i->operator[]("id")->intValue();
			result.push_back(dbadapter::row_to_object(id, *i));
		}
		return result;
	}
};

template<typename object, typename dbadapter>
object	Table<object, dbadapter>::byid(long objectid) {
	return dbadapter::row_to_object(objectid, rowbyid(objectid));
}

template<typename object, typename dbadapter>
long	Table<object, dbadapter>::add(const object& o) {
	return addrow(dbadapter::object_to_updatespec(o));
}

template<typename object, typename dbadapter>
void	Table<object, dbadapter>::update(long objectid, const object& o) {
	updaterow(objectid, dbadapter::object_to_updatespec(o));
}

} // namespace persistence
} // namespace astro

#endif /* _AstroPersistence_h */
