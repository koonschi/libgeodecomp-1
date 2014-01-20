#ifndef LIBGEODECOMP_GEOMETRY_PARTITIONS_SCOTCHPARTITION_H
#define LIBGEODECOMP_GEOMETRY_PARTITIONS_SCOTCHPARTITION_H

#include <libgeodecomp/geometry/partitions/partition.h>
#include "/usr/local/include/scotch.h"
#include <boost/timer.hpp>

namespace LibGeoDecomp {

template<int DIM>
class ScotchPartition : public Partition<DIM>
{
public:
    using Partition<DIM>::startOffsets;
    using Partition<DIM>::weights;

    inline ScotchPartition(
        const Coord<DIM>& origin = Coord<DIM>(),
        const Coord<DIM>& dimensions = Coord<DIM>(),
        const long& offset = 0,
        const std::vector<std::size_t>& weights = std::vector<std::size_t>(2)) :
        Partition<DIM>(offset, weights),
        origin(origin),
        dimensions(dimensions),
        cellNbr(dimensions[0] * dimensions[1])
        {
            indices = new SCOTCH_Num[cellNbr];
            regions = new Region<DIM>[weights.size()];
            /*boxes = new std::vector<std::pair <int,int> >[weights.size()];
            for(int i = 0;i<weights.size();++i){
                std::vector<std::pair <int,int> > blubber;
                boxes[i] = blubber;
                }*/
            boost::timer t;
            initIndices();
            std::cout << "\n" <<  t.elapsed() << std::endl;
            createRegions();
            std::cout << "\n" <<  t.elapsed() << std::endl;
        }

    Region<DIM> getRegion(const std::size_t node) const
    {
        return regions[node];
    }

private:
    Coord<DIM> origin;
    Coord<DIM> dimensions;
    SCOTCH_Num * indices;
    SCOTCH_Num const cellNbr;
    Region<DIM> * regions;

    std::vector<std::pair <int,int> > * boxes;

    void initIndices(){
        SCOTCH_Arch arch;
        SCOTCH_archInit(&arch);
        SCOTCH_Num * velotabArch;
        SCOTCH_Num const vertnbrArch = weights.size();
        velotabArch = new SCOTCH_Num[weights.size()];
        for(int i = 0;i<vertnbrArch;++i){
            velotabArch[i] = weights[i];
        }
        SCOTCH_archCmpltw (&arch,vertnbrArch,velotabArch);

        SCOTCH_Strat * straptr = SCOTCH_stratAlloc();;
        SCOTCH_stratInit(straptr);
        SCOTCH_stratGraphMapBuild(straptr,SCOTCH_STRATRECURSIVE,vertnbrArch,0);

        SCOTCH_Graph grafdat;
        SCOTCH_graphInit (&grafdat);

        SCOTCH_Num const edgenbrGra = 2 * (dimensions[0] * (dimensions[1] - 1) + (dimensions[0] - 1) * dimensions[1]);

        SCOTCH_Num * verttabGra;
        SCOTCH_Num * edgetabGra;
        verttabGra = new SCOTCH_Num[cellNbr + 1];
        edgetabGra = new SCOTCH_Num[edgenbrGra];


        int pointer = 0;
        for(int i = 0;i < cellNbr;++i){
            verttabGra[i] = pointer;
            if(i%dimensions[0] != 0){
                edgetabGra[pointer] = i-1;
                pointer++;
            }
            if(i%dimensions[0] != (dimensions[0]-1)){
                edgetabGra[pointer] = i+1;
                pointer++;
            }
            if(!(i < dimensions[0])){
                edgetabGra[pointer] = i-dimensions[0];
                pointer++;
            }
            if(!(i >= dimensions[0] * (dimensions[1]-1))){
                edgetabGra[pointer] = i+dimensions[0];
                pointer++;
            }
        }
        verttabGra[cellNbr] = pointer;


        if(SCOTCH_graphBuild(&grafdat,0,cellNbr,verttabGra,verttabGra +1,NULL,NULL,edgenbrGra, edgetabGra, NULL) != 0){
        }

        if(SCOTCH_graphCheck(&grafdat) != 0){
            //fixme error handling
            std::cerr << "graphCheck error" << std::endl;
        }

        SCOTCH_graphMap (&grafdat,&arch,straptr,indices);
    }

    void createRegions(){
        int rank = indices[0];
        int length = 0;
        int start = 0;
        for(int i = 1;i < cellNbr+1;++i){
            if(i == cellNbr+1 || rank == indices[i]){
                length++;
            } else {
                length++;
                regions[rank] << CoordBox<DIM>(Coord<DIM>(start%dimensions[0],
                                                          start/dimensions[1]),
                                               Coord<DIM>(length,1));
                rank = indices[i];
                start = i;
                length = 0;
            }
        }
    }

 };
}

#endif
