#ifndef LIBGEODECOMP_PARALLELIZATION_HPXSIMULATOR_HPXUPDATEGROUP_H
#define LIBGEODECOMP_PARALLELIZATION_HPXSIMULATOR_HPXUPDATEGROUP_H

#include <libgeodecomp/config.h>
#ifdef LIBGEODECOMP_WITH_HPX

#include <libgeodecomp/communication/hpxserializationwrapper.h>
#include <libgeodecomp/communication/hpxpatchlink.h>
#include <libgeodecomp/parallelization/updategroup.h>

namespace LibGeoDecomp {

// fixme: can't we reuse the MPI UpdateGroup here?
template <class CELL_TYPE>
class HPXUpdateGroup : public UpdateGroup<CELL_TYPE, HPXPatchLink>
{
public:
    friend class HiParSimulatorTest;
    friend class UpdateGroupPrototypeTest;
    friend class UpdateGroupTest;

    using typename UpdateGroup<CELL_TYPE, HPXPatchLink>::GridType;
    using typename UpdateGroup<CELL_TYPE, HPXPatchLink>::PatchAccepterVec;
    using typename UpdateGroup<CELL_TYPE, HPXPatchLink>::PatchProviderVec;
    using typename UpdateGroup<CELL_TYPE, HPXPatchLink>::PatchLinkAccepter;
    using typename UpdateGroup<CELL_TYPE, HPXPatchLink>::PatchLinkProvider;
    using typename UpdateGroup<CELL_TYPE, HPXPatchLink>::PatchLinkPtr;
    using typename UpdateGroup<CELL_TYPE, HPXPatchLink>::RegionVecMap;
    using typename UpdateGroup<CELL_TYPE, HPXPatchLink>::StepperType;
    using typename UpdateGroup<CELL_TYPE, HPXPatchLink>::Topology;

    using UpdateGroup<CELL_TYPE, HPXPatchLink>::init;
    using UpdateGroup<CELL_TYPE, HPXPatchLink>::partitionManager;
    using UpdateGroup<CELL_TYPE, HPXPatchLink>::patchLinks;
    using UpdateGroup<CELL_TYPE, HPXPatchLink>::rank;
    using UpdateGroup<CELL_TYPE, HPXPatchLink>::stepper;
    using UpdateGroup<CELL_TYPE, HPXPatchLink>::DIM;

    template<typename STEPPER>
    HPXUpdateGroup(
        boost::shared_ptr<Partition<DIM> > partition,
        const CoordBox<DIM>& box,
        const unsigned& ghostZoneWidth,
        boost::shared_ptr<Initializer<CELL_TYPE> > initializer,
        STEPPER *stepperType,
        PatchAccepterVec patchAcceptersGhost = PatchAccepterVec(),
        PatchAccepterVec patchAcceptersInner = PatchAccepterVec(),
        PatchProviderVec patchProvidersGhost = PatchProviderVec(),
        PatchProviderVec patchProvidersInner = PatchProviderVec(),
        std::string basename = "/HPXSimulator::UpdateGroup",
        int rank = hpx::get_locality_id()) :
        UpdateGroup<CELL_TYPE, HPXPatchLink>(ghostZoneWidth, initializer, rank),
        basename(basename)
    {
        init(
            partition,
            box,
            ghostZoneWidth,
            initializer,
            stepperType,
            patchAcceptersGhost,
            patchAcceptersInner,
            patchProvidersGhost,
            patchProvidersInner);
    }

private:
    std::string basename;

    std::vector<CoordBox<DIM> > gatherBoundingBoxes(boost::shared_ptr<Partition<DIM> > partition) const
    {
        std::size_t size = partition->getWeights().size();
        std::string broadcastName = basename + "/boundingBoxes";
        std::vector<CoordBox<DIM> > boundingBoxes;
        CoordBox<DIM> ownBoundingBox(partitionManager->ownRegion().boundingBox());
        return HPXReceiver<CoordBox<DIM> >::allGather(ownBoundingBox, rank, size, broadcastName);
    }

    virtual boost::shared_ptr<PatchLinkAccepter> makePatchLinkAccepter(int target, const Region<DIM>& region)
    {
        return boost::shared_ptr<typename HPXPatchLink<GridType>::Accepter>(
            new typename HPXPatchLink<GridType>::Accepter(
                region,
                basename,
                rank,
                target));

    }

    virtual boost::shared_ptr<PatchLinkProvider> makePatchLinkProvider(int source, const Region<DIM>& region)
    {
        return boost::shared_ptr<typename HPXPatchLink<GridType>::Provider>(
            new typename HPXPatchLink<GridType>::Provider(
                region,
                basename,
                source,
                rank));
    }

};

}

#endif
#endif