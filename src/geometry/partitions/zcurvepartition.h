#ifndef LIBGEODECOMP_GEOMETRY_PARTITIONS_ZCURVEPARTITION_H
#define LIBGEODECOMP_GEOMETRY_PARTITIONS_ZCURVEPARTITION_H

#include <libgeodecomp/geometry/coordbox.h>
#include <libgeodecomp/geometry/partitions/spacefillingcurve.h>
#include <libgeodecomp/geometry/topologies.h>
#include <libgeodecomp/storage/grid.h>

#include <bitset>
#include <boost/shared_ptr.hpp>
#include <sstream>
#include <stdexcept>
#include <list>

namespace LibGeoDecomp {

/**
 * Another space-filling curve, this one is based on a zip-zag pattern
 * (hence the name) and produces almost rectangular, albeit not
 * necessarily connected, subdomains.
 */
template<int DIMENSIONS>
class ZCurvePartition : public SpaceFillingCurve<DIMENSIONS>
{
    friend class ZCurvePartitionTest;
public:
    const static int DIM = DIMENSIONS;

    typedef std::vector<Coord<DIM> > CoordVector;
    typedef Grid<CoordVector, typename Topologies::Cube<DIM>::Topology> CacheType;
    typedef boost::shared_ptr<CacheType> Cache;
    typedef typename Topologies::Cube<DIM>::Topology Topology;

    class Square
    {
    public:
        inline Square(
            const Coord<DIM>& origin,
            const Coord<DIM> dimensions,
            unsigned quadrant) :
            origin(origin),
            dimensions(dimensions),
            quadrant(quadrant)
        {}

        inline std::string toString() const
        {
            std::stringstream s;
            s << "Square(origin:" << origin << ", dimensions:" << dimensions << ", quadrant: " << quadrant << ")";
            return s.str();
        }

        Coord<DIM> origin;
        Coord<DIM> dimensions;
        unsigned quadrant;
    };

    class Iterator : public SpaceFillingCurve<DIM>::Iterator
    {
    public:
        friend class ZCurvePartitionTest;

        using SpaceFillingCurve<DIM>::Iterator::cursor;
        using SpaceFillingCurve<DIM>::Iterator::endReached;
        using SpaceFillingCurve<DIM>::Iterator::hasTrivialDimensions;
        using SpaceFillingCurve<DIM>::Iterator::origin;
        using SpaceFillingCurve<DIM>::Iterator::sublevelState;

        static const int NUM_QUADRANTS = 1 << DIM;

        inline Iterator(
            const Coord<DIM>& origin,
            const Coord<DIM>& dimensions,
            unsigned pos = 0) :
            SpaceFillingCurve<DIM>::Iterator(origin, false)
        {
            squareStack.push_back(Square(origin, dimensions, 0));
            digDown(pos);
        }

        inline explicit Iterator(const Coord<DIM>& origin) :
            SpaceFillingCurve<DIM>::Iterator(origin, true)
        {}

        inline Iterator& operator++()
        {
            if (endReached) {
                return *this;
            }

            if (sublevelState == TRIVIAL) {
                operatorIncTrivial();
            } else {
                operatorIncCached();
            }
            return *this;
        }
    private:
        std::list<Square> squareStack;
        unsigned trivialSquareDirDim;
        unsigned trivialSquareCounter;
        Coord<DIM> cachedSquareOrigin;
        Coord<DIM> *cachedSquareCoordsIterator;
        Coord<DIM> *cachedSquareCoordsEnd;

        inline void operatorIncTrivial()
        {
            if (--trivialSquareCounter > 0) {
                cursor[trivialSquareDirDim]++;
            } else {
                digUpDown();
            }
        }

        inline void operatorIncCached()
        {
            cachedSquareCoordsIterator++;
            if (cachedSquareCoordsIterator != cachedSquareCoordsEnd) {
                cursor = cachedSquareOrigin + *cachedSquareCoordsIterator;
            } else {
                digUpDown();
            }
        }

        inline void digDown(int offset)
        {
            if (squareStack.empty()) {
                throw std::logic_error("cannot descend from empty squares stack");
            }
            Square currentSquare = squareStack.back();
            squareStack.pop_back();
            const Coord<DIM>& origin = currentSquare.origin;
            const Coord<DIM>& dimensions = currentSquare.dimensions;

            if (offset >= dimensions.prod()) {
                endReached = true;
                cursor = origin;
                return;
            }
            if (hasTrivialDimensions(dimensions)) {
                digDownTrivial(origin, dimensions, offset);
            } else if (isCached(dimensions)) {
                digDownCached(origin, dimensions, offset);
            } else {
                digDownRecursion(offset, currentSquare);
            }
        }

        inline void digDownTrivial(
            const Coord<DIM>& origin,
            const Coord<DIM>& dimensions,
            unsigned offset)
        {
            sublevelState = TRIVIAL;
            cursor = origin;

            trivialSquareDirDim = 0;
            for (int i = 1; i < DIM; ++i) {
                if (dimensions[i] > 1) {
                    trivialSquareDirDim = i;
                }
            }

            trivialSquareCounter = dimensions[trivialSquareDirDim] - offset;
            cursor[trivialSquareDirDim] += offset;

        }

        inline void digDownCached(
            const Coord<DIM>& origin,
            const Coord<DIM>& dimensions,
            unsigned offset)
        {
            sublevelState = CACHED;
            CoordVector& coords = (*ZCurvePartition<DIM>::coordsCache)[dimensions];
            cachedSquareOrigin = origin;
            cachedSquareCoordsIterator = &coords[offset];
            cachedSquareCoordsEnd      = &coords[0] + coords.size();
            cursor = cachedSquareOrigin + *cachedSquareCoordsIterator;
        }

        inline void digDownRecursion(unsigned offset, Square currentSquare)
        {
            const Coord<DIM>& dimensions = currentSquare.dimensions;
            Coord<DIM> halfDimensions = dimensions / 2;
            Coord<DIM> remainingDimensions = dimensions - halfDimensions;

            const int numQuadrants = ZCurvePartition<DIM>::Iterator::NUM_QUADRANTS;
            Coord<DIM> quadrantDims[numQuadrants];

            for (int i = 0; i < numQuadrants; ++i) {
                // high bits denote that the quadrant is in the upper
                // half in respect to that dimension, e.g. quadrant 2
                // would be (for 2D) in the lower half for dimension 0
                // but in the higher half for dimension 1.
                std::bitset<DIM> quadrantShift(i);
                Coord<DIM> quadrantDim;

                for (int d = 0; d < DIM; ++d) {
                    quadrantDim[d] = quadrantShift[d]?
                        remainingDimensions[d] :
                        halfDimensions[d];
                }

                quadrantDims[i] = quadrantDim;
            }

            unsigned accuSizes[numQuadrants];
            accuSizes[0] = 0;
            for (int i = 1; i < numQuadrants; ++i) {
                accuSizes[i] = accuSizes[i - 1] + quadrantDims[i - 1].prod();
            }

            unsigned pos = offset + accuSizes[currentSquare.quadrant];
            unsigned index = std::upper_bound(
                accuSizes,
                accuSizes + numQuadrants,
                pos) - accuSizes - 1;

            if (index >= (1 << DIM)) {
                throw std::logic_error("offset too large?");
            }

            unsigned newOffset = pos - accuSizes[index];
            Coord<DIM> newDimensions = quadrantDims[index];
            Coord<DIM> newOrigin;

            std::bitset<DIM> quadrantShift(index);
            for (int d = 0; d < DIM; ++d) {
                newOrigin[d] = quadrantShift[d]? halfDimensions[d] : 0;
            }
            newOrigin += currentSquare.origin;

            currentSquare.quadrant = index;
            squareStack.push_back(currentSquare);

            Square newSquare(newOrigin, newDimensions, 0);
            squareStack.push_back(newSquare);

            digDown(newOffset);
        }

        inline void digUp()
        {
            while (!squareStack.empty() &&
                   (squareStack.back().quadrant ==
                    (ZCurvePartition<DIM>::Iterator::NUM_QUADRANTS - 1))) {
                squareStack.pop_back();
            }

            if (squareStack.empty()) {
                endReached = true;
                cursor = origin;
            } else {
                squareStack.back().quadrant++;
            }
        }

        inline void digUpDown()
        {
            digUp();
            if (endReached) {
                return;
            }
            digDown(0);
        }

        inline bool isCached(const Coord<DIM>& dimensions) const
        {
            bool ret = true;
            for (int i = 0; i < DIM; ++i)
                ret &= dimensions[i] < maxCachedDimensions[i];
            return ret;
        }
    };

    inline explicit ZCurvePartition(
        const Coord<DIM>& origin = Coord<DIM>(),
        const Coord<DIM>& dimensions = Coord<DIM>(),
        const long& offset = 0,
        const std::vector<std::size_t>& weights = std::vector<std::size_t>(2),
        const boost::shared_ptr<Adjacency>& /* unused: adjacency */ = boost::make_shared<RegionBasedAdjacency>()) :
        SpaceFillingCurve<DIM>(offset, weights),
        origin(origin),
        dimensions(dimensions)
    {}

    inline Iterator operator[](unsigned i) const
    {
        return Iterator(origin, dimensions, i);
    }

    inline Iterator begin() const
    {
        return (*this)[0];
    }

    inline Iterator end() const
    {
        return Iterator(origin);
    }

    inline Region<DIM> getRegion(const std::size_t node) const
    {
        return Region<DIM>(
            (*this)[startOffsets[node + 0]],
            (*this)[startOffsets[node + 1]]);
    }

    static inline bool fillCaches()
    {
        // store squares of at most maxDim in size. the division by
        // DIM^2 is a trick to keep the cache small if DIM is large.
        Coord<DIM> maxDim = Coord<DIM>::diagonal(68 / DIM / DIM);
        ZCurvePartition<DIM>::coordsCache.reset(
            Grid<CoordVector, Topologies::Cube<DIM> >(maxDim));

        CoordBox<DIM> box(Coord<DIM>(), maxDim);
        for (typename CoordBox<DIM>::Iterator iter = box.begin(); iter != box.end(); ++iter) {
            Coord<DIM> dim = *iter;
            if (!hasTrivialDimensions(dim)) {
                CoordVector coords;
                Iterator end((Coord<DIM>()));
                for (Iterator i(Coord<DIM>(), dim, 0); i != end; ++i) {
                    coords.push_back(*i);
                }
                (*coordsCache)[dim] = coords;
            }
        }

        ZCurvePartition<DIM>::maxCachedDimensions = maxDim;
        return true;
    }

private:
    using SpaceFillingCurve<DIM>::startOffsets;

    static Cache coordsCache;
    static Coord<DIMENSIONS> maxCachedDimensions;
    static bool cachesInitialized;

    Coord<DIM> origin;
    Coord<DIM> dimensions;
};

template<int DIM>
typename ZCurvePartition<DIM>::Cache ZCurvePartition<DIM>::coordsCache;

template<int DIM>
Coord<DIM> ZCurvePartition<DIM>::maxCachedDimensions;

template<int DIM>
bool ZCurvePartition<DIM>::cachesInitialized = ZCurvePartition<DIM>::fillCaches();

}

#endif
