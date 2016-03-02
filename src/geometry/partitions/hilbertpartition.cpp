#include <libgeodecomp/geometry/partitions/hilbertpartition.h>

namespace LibGeoDecomp {

HilbertPartition::Form HilbertPartition::squareFormTransitions[4][4] = {
    {LL_TO_UL, LL_TO_LR, LL_TO_LR, UR_TO_LR}, // LL_TO_LR
    {LL_TO_LR, LL_TO_UL, LL_TO_UL, UR_TO_UL}, // LL_TO_UL
    {UR_TO_UL, UR_TO_LR, UR_TO_LR, LL_TO_LR}, // UR_TO_LR
    {UR_TO_LR, UR_TO_UL, UR_TO_UL, LL_TO_UL}  // UR_TO_UL
};

// (sub-)sectors:
// 01
// 23
int HilbertPartition::squareSectorTransitions[4][4] = {
    {2, 0, 1, 3}, // LL_TO_LR
    {2, 3, 1, 0}, // LL_TO_UL
    {1, 0, 2, 3}, // UR_TO_LR
    {1, 3, 2, 0}  // UR_TO_UL
};

boost::shared_ptr<Grid<std::vector<Coord<2> >, Topologies::Cube<3>::Topology> > HilbertPartition::squareCoordsCache;
Coord<2> HilbertPartition::maxCachedDimensions;
bool HilbertPartition::cachesInitialized = HilbertPartition::fillCaches();

}
