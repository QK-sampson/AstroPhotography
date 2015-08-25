/*
 * Function.cpp -- algorithms to extract a background gradient from an image
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <AstroBackground.h>
#include <AstroFormat.h>
#include <AstroFilter.h>
#include <AstroUtils.h>
#include <cmath>
#include <set>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_ACCELERATE_ACCELERATE_H
#include <Accelerate/Accelerate.h>
#else
#include <lapack.h>
#endif /* HAVE_ACCELERATE_ACCELERATE_H */

using namespace astro::image;
using namespace astro::image::filter;

namespace astro {
namespace adapter {

//////////////////////////////////////////////////////////////////////
// FunctionBase implementation
//////////////////////////////////////////////////////////////////////

FunctionBase::FunctionBase(const FunctionBase& other) :
	_symmetric(other.symmetric()), _center(other.center()) {
	_gradient = other.gradient();
	_scalefactor = other.scalefactor();
}

double	FunctionBase::evaluate(const ImagePoint& point) const {
	return evaluate(Point(point.x(), point.y()));
}

double	FunctionBase::evaluate(int x, int y) const {
	return evaluate(Point(x, y));
}

double	FunctionBase::operator()(const Point& point) const {
	return evaluate(point);
}

double	FunctionBase::operator()(const ImagePoint& point) const {
	return evaluate(point);
}

double	FunctionBase::operator()(int x, int y) const {
	return evaluate(x, y);
}

std::string	FunctionBase::toString() const {
	return stringprintf("[gradient=%s,symmetric=%s,scalefactor=%.3f]",
		(gradient()) ? "YES" : "NO", (symmetric()) ? "YES" : "NO",
		scalefactor());
}

//////////////////////////////////////////////////////////////////////
// LinearFunction implementation
//////////////////////////////////////////////////////////////////////

LinearFunction::LinearFunction(const ImagePoint& point, bool symmetric)
	: FunctionBase(point, symmetric) {
	for (int i = 0; i < 3; i++) {
		a[i] = 0;
	}
}

LinearFunction::LinearFunction(const LinearFunction& other) :
	FunctionBase(other) {
	for (int i = 0; i < 3; i++) {
		a[i] = other.a[i];
	}
}

double  LinearFunction::evaluate(const Point& point) const {
	double	value = a[2];
	if (gradient() && (!symmetric())) {
		double	deltax = point.x() - center().x();
		double	deltay = point.y() - center().y();
		value += (deltax * a[0] + deltay * a[1]);
	}
	return scalefactor() * value;
}

inline static double	sqr(const double& x) {
	return x * x;
}

double	LinearFunction::norm() const {
	double	result = 0;
	result += sqr(center().x() * a[0]);
	result += sqr(center().y() * a[1]);
	result += sqr(a[2]);
	return sqrt(result);
}

LinearFunction      LinearFunction::operator+(const LinearFunction& other) {
	LinearFunction      result(*this);
	for (unsigned int i = 0; i < 3; i++) {
		result.a[i] = a[i] + other.a[i];
	}
	return result;
}

LinearFunction&     LinearFunction::operator=(
	const LinearFunction& other) {
	for (int i = 0; i < 3; i++) {
		a[i] = other.a[i];
	}
	return *this;
}

/**
 * \brief read only access to coefficients
 */
double	LinearFunction::operator[](int i) const {
	if ((i < 0) || (i > 2)) {
		throw std::range_error("index out of range");
	}
	return a[i];
}

/**
 * \brief modifying access to coefficients
 */
double&	LinearFunction::operator[](int i) {
	if ((i < 0) || (i > 2)) {
		throw std::range_error("index out of range");
	}
	return a[i];
}

/**
 * \brief Compute the best possible coefficients from a data set
 */
void	LinearFunction::reduce(const std::vector<doublevaluepair>& values) {
	// build a linear system for coefficients a[3]
	int	m = values.size();
	int	n = 3;
	double	A[3 * m * n];
	double	b[m];
	int	line = 0;
	std::vector<doublevaluepair>::const_iterator	vp;
	for (vp = values.begin(); vp != values.end(); vp++, line++) {
		A[line + 0 * m] = vp->first.x() - center().x();
		A[line + 1 * m] = vp->first.y() - center().x();
		A[line + 2 * m] = 1;
		b[line] = vp->second;
	}

	// set up the lapack stuff
	char	trans = 'N';
	int	nrhs = 1;
	int	lda = m;
	int	ldb = m;
	int	lwork = -1;
	int	info = 0;

	// first find out how large we have to make the work area
	double	x;
	dgels_(&trans, &m, &n, &nrhs, A, &lda, b, &ldb, &x, &lwork, &info);
	if (info != 0) {
		std::string	msg = stringprintf("dgels cannot determine "
			"work area size: %d", info);
		debug(LOG_ERR, DEBUG_LOG, 0, "%s", msg.c_str());
		throw std::runtime_error(msg);
	}
	lwork = x;
	debug(LOG_DEBUG, DEBUG_LOG, 0, "work area size: %d", lwork);

	// now allocate memory and get the solution
	double	work[lwork];
	dgels_(&trans, &m, &n, &nrhs, A, &lda, b, &ldb, work, &lwork, &info);
	if (info != 0) {
		std::string	msg = stringprintf("dgels cannot solve "
			"equations: %d", info);
		debug(LOG_ERR, DEBUG_LOG, 0, "%s", msg.c_str());
		throw std::runtime_error(msg);
	}

	// copy the result vector
	for (int i = 0; i < 3; i++) {
		a[i] = b[i];
	}
	debug(LOG_DEBUG, DEBUG_LOG, 0, "Linear function found: %s",
		toString().c_str());
}

/**
 * \brief Create a linear function from a set of value pairs
 */
LinearFunction::LinearFunction(const ImagePoint& center, bool symmetric,
	const std::vector<LinearFunction::doublevaluepair>& values)
	: FunctionBase(center, symmetric) {
	a[0] = a[1] = a[2] = 0;
	reduce(values);
}

/**
 * \brief Text representation of a linear form
 */
std::string	LinearFunction::toString() const {
	return FunctionBase::toString()
		+ stringprintf("%f * dx + %f * dy + %f", a[0], a[1], a[2]);
}

//////////////////////////////////////////////////////////////////////
// QuadraticFunction implementation
//////////////////////////////////////////////////////////////////////

/**
 * \brief 
 */
QuadraticFunction::QuadraticFunction(const ImagePoint& center,
	bool symmetric) : LinearFunction(center, symmetric) {
	for (unsigned int i = 0; i < 3; i++) {
		q[i] = 0;
	}
}

QuadraticFunction::QuadraticFunction(const LinearFunction& lin)
	: LinearFunction(lin) {
	for (unsigned int i = 0; i < 3; i++) {
		q[i] = 0;
	}
}

double	QuadraticFunction::evaluate(const Point& point) const {
	double	value = LinearFunction::evaluate(point);
	if (gradient()) {
		double	deltax = point.x() - center().x();
		double	deltay = point.y() - center().y();
		value += q[0] * (sqr(deltax) + sqr(deltay));
		if (!symmetric()) {
			value += q[1] * deltax * deltay
				+ q[2] * (sqr(deltax) - sqr(deltay));
		}
	}
	return value;
}

double	QuadraticFunction::operator[](int i) const {
	if (i < 3) {
		return LinearFunction::operator[](i);
	}
	if (i > 5) {
		std::runtime_error("index out of range");
	}
	return q[i - 3];
}

double	QuadraticFunction::norm() const {
	double	s = sqr(LinearFunction::norm());
	for (int i = 0; i < 3; i++) {
		s += sqr(q[i]);
	}
	return sqrt(s);
}

void	QuadraticFunction::reduce(const std::vector<FunctionBase::doublevaluepair>& /* values */) {
	throw std::runtime_error("QuadraticFunction::reduce not implemented");
}

double&	QuadraticFunction::operator[](int i) {
	if (i < 3) {
		return LinearFunction::operator[](i);
	}
	if (i > 5) {
		std::runtime_error("index out of range");
	}
	return q[i - 3];
}

QuadraticFunction	QuadraticFunction::operator+(
	const QuadraticFunction& other) {
	QuadraticFunction	result(center(),
					symmetric() || other.symmetric());
	for (unsigned int i = 0; i < 6; i++) {
		result[i] += (*this)[i] + other[i];
	}
	return result;
}

QuadraticFunction	QuadraticFunction::operator+(
	const LinearFunction& other) {
	QuadraticFunction	result(center(),
					symmetric() || other.symmetric());
	for (unsigned int i = 0; i < 3; i++) {
		result[i] += (*this)[i] + other[i];
	}
	for (unsigned int i = 3; i < 5; i++) {
		result[i] += (*this)[i];
	}
	return result;
}

QuadraticFunction&	QuadraticFunction::operator=(
	const QuadraticFunction& other) {
	LinearFunction::operator=(other);
	for (unsigned int i = 0; i < 3; i++) {
		q[i] = other.q[i];
	}
	return *this;
}

QuadraticFunction&	QuadraticFunction::operator=(
	const LinearFunction& other) {
	LinearFunction::operator=(other);
	return *this;
}

std::string	QuadraticFunction::toString() const {
	return LinearFunction::toString()
		+ stringprintf("[%.6f, %.6f, %.6f]", q[0], q[1], q[2]);
} 

//////////////////////////////////////////////////////////////////////
// arithmetic operators for FunctionPtr
//////////////////////////////////////////////////////////////////////
FunctionPtr	operator+(const FunctionPtr& a, const FunctionPtr& b) {
	LinearFunction	*la = dynamic_cast<LinearFunction*>(&*a);
	LinearFunction	*lb = dynamic_cast<LinearFunction*>(&*b);
	QuadraticFunction	*qa = dynamic_cast<QuadraticFunction*>(&*a);
	QuadraticFunction	*qb = dynamic_cast<QuadraticFunction*>(&*b);
	if (qa == NULL) {
		if (qb == NULL) {
			debug(LOG_DEBUG, DEBUG_LOG, 0, "linear + linear");
			return FunctionPtr(new LinearFunction(*la + *lb));
		} else {
			debug(LOG_DEBUG, DEBUG_LOG, 0, "linear + quadratic");
			QuadraticFunction	q(*la);
			return FunctionPtr(new QuadraticFunction(q + *qb));
		}
	} else {
		if (qb == NULL) {
			debug(LOG_DEBUG, DEBUG_LOG, 0, "quadratic + linear");
			QuadraticFunction	q(*lb);
			return FunctionPtr(new QuadraticFunction(*qa + q));
		} else {
			debug(LOG_DEBUG, DEBUG_LOG, 0, "quadratic + quadratic");
			return FunctionPtr(new QuadraticFunction(*qa + *qb));
		}
	}
}

} // namespace image
} // namespace astro
