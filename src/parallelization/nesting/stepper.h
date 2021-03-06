#ifndef LIBGEODECOMP_PARALLELIZATION_NESTING_STEPPER_H
#define LIBGEODECOMP_PARALLELIZATION_NESTING_STEPPER_H

#include <boost/shared_ptr.hpp>
#include <deque>

#include <libgeodecomp/geometry/partitionmanager.h>
#include <libgeodecomp/io/initializer.h>
#include <libgeodecomp/misc/chronometer.h>
#include <libgeodecomp/parallelization/nesting/offsethelper.h>
#include <libgeodecomp/storage/gridtypeselector.h>
#include <libgeodecomp/storage/patchaccepter.h>
#include <libgeodecomp/storage/patchprovider.h>

namespace LibGeoDecomp {

/**
 * Abstract interface class. Steppers contain some arbitrary region of
 * the grid which they can update. IO and ghostzone communication are
 * handled via PatchAccepter and PatchProvider objects.
 */
template<typename CELL_TYPE>
class Stepper
{
public:
    friend class StepperTest;

    enum PatchType {GHOST=0, INNER_SET=1};
    typedef typename APITraits::SelectTopology<CELL_TYPE>::Value Topology;
    const static int DIM = Topology::DIM;

    typedef typename APITraits::SelectSoA<CELL_TYPE>::Value SupportsSoA;
    typedef typename GridTypeSelector<CELL_TYPE, Topology, true, SupportsSoA>::Value GridType;

    typedef PartitionManager<Topology> PartitionManagerType;
    typedef boost::shared_ptr<PatchProvider<GridType> > PatchProviderPtr;
    typedef boost::shared_ptr<PatchAccepter<GridType> > PatchAccepterPtr;
    typedef std::deque<PatchProviderPtr> PatchProviderList;
    typedef std::deque<PatchAccepterPtr> PatchAccepterList;
    typedef std::vector<PatchAccepterPtr> PatchAccepterVec;
    typedef std::vector<PatchProviderPtr> PatchProviderVec;

    inline Stepper(
        const boost::shared_ptr<PartitionManagerType>& partitionManager,
        boost::shared_ptr<Initializer<CELL_TYPE> > initializer) :
        partitionManager(partitionManager),
        initializer(initializer)
    {}

    virtual ~Stepper()
    {}

    virtual void update(std::size_t nanoSteps) = 0;

    virtual const GridType& grid() const = 0;

    /**
     * returns current step and nanoStep
     */
    virtual std::pair<std::size_t, std::size_t> currentStep() const = 0;

    void addPatchProvider(
        const PatchProviderPtr& patchProvider,
        const PatchType& patchType)
    {
        patchProviders[patchType].push_back(patchProvider);
    }

    void addPatchAccepter(
        const PatchAccepterPtr& patchAccepter,
        const PatchType& patchType)
    {
        patchAccepters[patchType].push_back(patchAccepter);
    }

    const Chronometer& statistics() const
    {
        return chronometer;
    }

protected:
    boost::shared_ptr<PartitionManagerType> partitionManager;
    boost::shared_ptr<Initializer<CELL_TYPE> > initializer;
    PatchProviderList patchProviders[2];
    PatchAccepterList patchAccepters[2];
    Chronometer chronometer;

    /**
     * calculates a (mostly) suitable offset which (in conjuction with
     * a DisplacedGrid) avoids having grids with a size equal to the
     * whole simulation area on torus topologies.
     */
    inline void guessOffset(Coord<DIM> *offset, Coord<DIM> *dimensions)
    {
        const CoordBox<DIM>& boundingBox =
            partitionManager->ownRegion().boundingBox();
        OffsetHelper<DIM - 1, DIM, Topology>()(
            offset,
            dimensions,
            boundingBox,
            initializer->gridBox(),
            partitionManager->getGhostZoneWidth());
    }
};

}

#endif
