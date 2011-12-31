#ifndef _libgeodecomp_misc_typetraits_h_
#define _libgeodecomp_misc_typetraits_h_

#include <boost/type_traits.hpp>

namespace LibGeoDecomp {

// fixme: remove
template<class MARKER>
class ProvidesStreakIterator : public boost::false_type 
{};

/**
 * Specifies whether a cell provides an interface to update a streak
 * of cells in a row, good for including vectorized user code.
 */
template<class CELL_TYPE>
class ProvidesStreakUpdate : public boost::false_type 
{};

}

#endif
