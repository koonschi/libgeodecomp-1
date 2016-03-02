#ifndef LIBGEODECOMP_GEOMETRY_PARTITIONS_RECURSIVEBISECTIONPARTITION_H
#define LIBGEODECOMP_GEOMETRY_PARTITIONS_RECURSIVEBISECTIONPARTITION_H

#include <libgeodecomp/geometry/floatcoord.h>
#include <libgeodecomp/geometry/partitions/partition.h>
#include <libgeodecomp/misc/math.h>

namespace LibGeoDecomp {

/**
 * This class implements a recursive weighted coordinate bisection. It
 * yields perfectly rectangular domains which can be acutely tuned to
 * match load profiles, but small changes in the load vector may lead
 * to huge communication volumes for rebalanciation.
 */
template<int DIM>
class RecursiveBisectionPartition : public Partition<DIM>
{
    friend class RecursiveBisectionPartitionTest;
public:
    typedef std::vector<std::size_t> SizeTVec;

    inline explicit RecursiveBisectionPartition(
        const Coord<DIM>& origin = Coord<DIM>(),
        const Coord<DIM>& dimensions = Coord<DIM>(),
        const long& offset = 0,
        const SizeTVec weights = SizeTVec(),
        const boost::shared_ptr<Adjacency>& adjacency = boost::make_shared<RegionBasedAdjacency>(),
        const Coord<DIM>& dimWeights = Coord<DIM>::diagonal(1)) :
        Partition<DIM>(0, weights),
        origin(origin),
        dimensions(dimensions),
        dimWeights(dimWeights)
    {
        if (dimensions.prod() == 0) {
            throw std::invalid_argument("size of simulation space may not be zero");
        }
    }

    inline Region<DIM> getRegion(const std::size_t i) const
    {
        CoordBox<DIM> cuboid = searchNodeCuboid(
            startOffsets.begin(),
            startOffsets.end() - 1,
            startOffsets.begin() + i,
            CoordBox<DIM>(origin, dimensions));

        Region<DIM> r;
        r << cuboid;
        return r;
    }

private:
    using Partition<DIM>::startOffsets;

    Coord<DIM> origin;
    Coord<DIM> dimensions;
    Coord<DIM> dimWeights;

    /**
     * returns the CoordBox which belongs to the node whose weight is
     * being pointed to by the iterator node. We assume that all
     * regions of the nodes from begin to end combined for the
     * CoordBox box.
     */
    CoordBox<DIM> searchNodeCuboid(
        const SizeTVec::const_iterator& begin,
        const SizeTVec::const_iterator& end,
        const SizeTVec::const_iterator& node,
        const CoordBox<DIM>& box) const
    {
        if (std::distance(begin, end) == 1) {
            return box;
        }

        std::size_t halfWeight = (*begin + *end) / 2;

        SizeTVec::const_iterator approxMiddle = std::lower_bound(
            begin,
            end,
            halfWeight);
        if (*approxMiddle != halfWeight) {
            std::size_t delta1 = *approxMiddle - halfWeight;
            SizeTVec::const_iterator predecessor = approxMiddle - 1;
            std::size_t delta2 = halfWeight - *predecessor;
            if (delta2 < delta1) {
                approxMiddle = predecessor;
            }
        }

        double ratio = 1.0 * (*approxMiddle - *begin) / (*end - *begin);
        CoordBox<DIM> newBoxes[2];
        splitBox(newBoxes, box, ratio);

        if (*node < *approxMiddle) {
            return searchNodeCuboid(begin, approxMiddle, node, newBoxes[0]);
        } else {
            return searchNodeCuboid(approxMiddle, end, node, newBoxes[1]);
        }
    }

    inline void splitBox(
        CoordBox<DIM> *newBoxes,
        const CoordBox<DIM>& oldBox,
        double ratio) const
    {
        newBoxes[0] = oldBox;
        newBoxes[1] = oldBox;

        int longestDim = 0;
        const Coord<DIM>& dim = oldBox.dimensions;
        for (int i = 1; i < DIM; ++i) {
            if ((dim[i] * dimWeights[i]) > (dim[longestDim] * dimWeights[longestDim])) {
                longestDim = i;
            }
        }

        int offset = round(ratio * dim[longestDim]);
        int remainder = dim[longestDim] - offset;
        newBoxes[0].dimensions[longestDim] = offset;
        newBoxes[1].dimensions[longestDim] = remainder;
        newBoxes[1].origin[longestDim] += offset;
    }
};

template<typename _CharT, typename _Traits, int _Dim>
std::basic_ostream<_CharT, _Traits>&
operator<<(std::basic_ostream<_CharT, _Traits>& __os,
           const typename RecursiveBisectionPartition<_Dim>::Iterator& iter)
{
    __os << iter.toString();
    return __os;
}

}

#endif
