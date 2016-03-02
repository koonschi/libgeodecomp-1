#ifndef LIBGEODECOMP_GEOMETRY_PARTITIONS_PTSCOTCHPARTITION_H
#define LIBGEODECOMP_GEOMETRY_PARTITIONS_PTSCOTCHPARTITION_H

#include <libgeodecomp/config.h>
#include <libgeodecomp/geometry/partitions/partition.h>

#ifdef LIBGEODECOMP_WITH_SCOTCH
#ifdef LIBGEODECOMP_WITH_MPI

#include <mpi.h>
#include <ptscotch.h>

namespace LibGeoDecomp {

/**
 * This class decomposes structured grids via PT-SCOTCH. It exports a
 * graph that corresponds to the structured grid's topology to SCOTCH
 * and then assembles the coordinate lists into Regions.
 */
template<int DIM>
class PTScotchPartition : public Partition<DIM>
{
public:
    using Partition<DIM>::startOffsets;
    using Partition<DIM>::weights;

    inline explicit PTScotchPartition(
        const Coord<DIM>& origin = Coord<DIM>(),
        const Coord<DIM>& dimensions = Coord<DIM>(),
        const long& offset = 0,
        const std::vector<std::size_t>& weights = std::vector<std::size_t>(2)) :
        Partition<DIM>(offset, weights),
        origin(origin),
        dimensions(dimensions),
        cellNbr(dimensions.prod())
    {
        std::vector<SCOTCH_Num> indices(cellNbr);
        initIndices(&indices[0]);
        regions.resize(weights.size());
        createRegions(&indices[0]);
    }

    Region<DIM> getRegion(const std::size_t node) const
    {
        return regions[node];
    }

private:
    Coord<DIM> origin;
    Coord<DIM> dimensions;
    SCOTCH_Num cellNbr;
    std::vector<Region<DIM> > regions;

    void initIndices(SCOTCH_Num *indices)
    {
        SCOTCH_Arch arch;
        SCOTCH_archInit(&arch);
        SCOTCH_Num * velotabArch;
        SCOTCH_Num const vertnbrArch = weights.size();
        velotabArch = new SCOTCH_Num[weights.size()];
        for(int i = 0; i < vertnbrArch; ++i){
            velotabArch[i] = weights[i];
        }
        SCOTCH_archCmpltw(&arch, vertnbrArch, velotabArch);

        SCOTCH_Graph grafdat;
        SCOTCH_graphInit(&grafdat);

        SCOTCH_Num edgenbrGra = 2 * (dimensions[0] *
                                     (dimensions[1] - 1) +
                                     (dimensions[0] - 1) * dimensions[1]);
        if(DIM == 3){
            edgenbrGra = edgenbrGra
                * dimensions[2]
                + 2 * (dimensions[0] *
                       dimensions[1] * (dimensions[2] - 1));
        }

        SCOTCH_Num *verttabGra;
        SCOTCH_Num *edgetabGra;
        verttabGra = new SCOTCH_Num[cellNbr + 1];
        edgetabGra = new SCOTCH_Num[edgenbrGra];

        int pointer = 0;
        int xyArea = dimensions[0] * dimensions[1];
        for(int i = 0; i < cellNbr; ++i){
            verttabGra[i] = pointer;
            if(i % dimensions[0] != 0){
                edgetabGra[pointer] = i - 1;
                pointer++;
            }
            if(i % dimensions[0] != (dimensions[0] - 1)){
                edgetabGra[pointer] = i + 1;
                pointer++;
            }
            if(!((i % xyArea) < dimensions[0])){
                edgetabGra[pointer] = i - dimensions[0];
                pointer++;
            }
            if(!((i % xyArea) >= dimensions[0] * (dimensions[1] - 1))){
                edgetabGra[pointer] = i + dimensions[0];
                pointer++;
            }
            if(DIM == 3 && i >= (dimensions[0] * dimensions[1])){
                edgetabGra[pointer] = i - (dimensions[0] * dimensions[1]);
                pointer++;
            }
            if(DIM == 3 && i < (dimensions[0] *
                                dimensions[1]) * (dimensions[2] - 1)){
                edgetabGra[pointer] = i + (dimensions[0] * dimensions[1]);
                pointer++;
            }
        }
        verttabGra[cellNbr] = pointer;

        SCOTCH_graphBuild(&grafdat,
                          0,
                          cellNbr,
                          verttabGra,
                          0,
                          0,
                          0,
                          edgenbrGra,
                          edgetabGra,
                          0);

        SCOTCH_Strat *straptr = SCOTCH_stratAlloc();;
        SCOTCH_stratInit(straptr);

        SCOTCH_graphMap(&grafdat, &arch, straptr, indices);

        SCOTCH_archExit(&arch);
        SCOTCH_graphExit(&grafdat);
        SCOTCH_stratExit(straptr);
        delete[] velotabArch;
        delete[] verttabGra;
        delete[] edgetabGra;

    }

    void createRegions(SCOTCH_Num *indices)
    {
        int rank = indices[0];
        int length = 0;
        int start = 0;
        for(int i = 1; i < cellNbr + 1; ++i){
            if((rank == indices[i]) &&
               (i < cellNbr) && (i % dimensions[0] != 0)){
                length++;
            } else {
                length++;
                Coord<DIM> startCoord;
                Coord<DIM> lengthCoord;
                lengthCoord[0] = length;
                lengthCoord[1] = 1;
                if(DIM == 3){
                    startCoord[0] = origin[0] +
                        (start % (dimensions[0] * dimensions[1])) %
                        dimensions[0];
                    startCoord[1] = origin[1] +
                        (start % (dimensions[0] * dimensions[1])) /
                        dimensions[0];
                    startCoord[2] = origin[2] + start /
                        (dimensions[0] *
                         dimensions[1]);
                    lengthCoord[2] = 1;
                } else {
                    startCoord[0] = origin[0] + start % dimensions[0];
                    startCoord[1] = origin[1] + start / dimensions[0];
                }


                regions[rank] <<
                    CoordBox<DIM>(startCoord, lengthCoord);
                rank = indices[i];
                start = i;
                length = 0;
            }
        }
    }
 };
}

#endif

#endif

#endif
