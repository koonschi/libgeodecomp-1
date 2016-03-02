#ifndef LIBGEODECOMP_GEOMETRY_PARTITIONS_STRIPINGPARTITION_H
#define LIBGEODECOMP_GEOMETRY_PARTITIONS_STRIPINGPARTITION_H

#include <libgeodecomp/geometry/coord.h>
#include <libgeodecomp/geometry/coordbox.h>
#include <libgeodecomp/geometry/partitions/spacefillingcurve.h>

#include <sstream>

namespace LibGeoDecomp {

/**
 * Implements a domain decomposition scheme based on block stripes,
 * hence the name. Spatial coordinates are simple ordered linearly via
 * their components. Technically this is a non-continuous space
 * filling curve.
 */
template<int DIMENSIONS>
class StripingPartition : public SpaceFillingCurve<DIMENSIONS>
{
public:
    friend class StripingPartitionTest;
    const static int DIM = DIMENSIONS;
    typedef typename CoordBox<DIM>::Iterator Iterator;

    explicit StripingPartition(
        const Coord<DIM>& origin = Coord<DIM>(),
        const Coord<DIM>& newDimensions = Coord<DIM>(),
        const long& offset = 0,
        const std::vector<std::size_t>& weights = std::vector<std::size_t>(2),
        const boost::shared_ptr<Adjacency>& /* unused: adjacency */ = boost::make_shared<RegionBasedAdjacency>()) :
        SpaceFillingCurve<DIM>(offset, weights),
        origin(origin),
        dimensions(newDimensions)
    {
        if (dimensions.prod() == 0) {
            // set all dimensions to 1 except for the last one to
            // avoid division by 0 in operator[]
            dimensions = Coord<DIM>::diagonal(1);
            dimensions[DIM - 1] = 0;
        }
    }

    Iterator begin() const
    {
        return Iterator(origin, origin, dimensions);
    }

    Iterator end() const
    {
        Coord<DIM> endOffset;
        endOffset[DIM - 1] = dimensions[DIM - 1];
        return Iterator(origin, origin + endOffset, dimensions);
    }

    inline Region<DIM> getRegion(const std::size_t node) const
    {
        return Region<DIM>(
            (*this)[startOffsets[node + 0]],
            (*this)[startOffsets[node + 1]]);
    }

    Iterator operator[](unsigned pos) const
    {
        Coord<DIM> cursor = dimensions.indexToCoord(pos) + origin;
        return Iterator(origin, cursor, dimensions);
    }


private:
    using SpaceFillingCurve<DIMENSIONS>::startOffsets;

    Coord<DIM> origin;
    Coord<DIM> dimensions;
};

template<typename _CharT, typename _Traits, int _Dim>
std::basic_ostream<_CharT, _Traits>&
operator<<(std::basic_ostream<_CharT, _Traits>& __os,
           const typename StripingPartition<_Dim>::Iterator& iter)
{
    __os << iter.toString();
    return __os;
}

}

#endif
