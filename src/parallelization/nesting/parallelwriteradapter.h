#ifndef LIBGEODECOMP_PARALLELIZATION_NESTING_PARALLELWRITERADAPTER_H
#define LIBGEODECOMP_PARALLELIZATION_NESTING_PARALLELWRITERADAPTER_H

#include <libgeodecomp/io/parallelwriter.h>
#include <libgeodecomp/storage/patchaccepter.h>

namespace LibGeoDecomp {

template<typename CELL_TYPE, typename PARTITION, typename STEPPER>
class HiParSimulator;

/**
 * ParallelWriterAdapter translates the interface of a ParallelWriter
 * to a PatchAccepter, so that we can treat IO similarly to sending
 * ghost zones.
 */
template<typename GRID_TYPE, typename CELL_TYPE>
class ParallelWriterAdapter : public PatchAccepter<GRID_TYPE>
{
public:
    typedef typename APITraits::SelectTopology<CELL_TYPE>::Value Topology;

    static const unsigned NANO_STEPS = APITraits::SelectNanoSteps<CELL_TYPE>::VALUE;

    using PatchAccepter<GRID_TYPE>::checkNanoStepPut;
    using PatchAccepter<GRID_TYPE>::pushRequest;
    using PatchAccepter<GRID_TYPE>::requestedNanoSteps;

    ParallelWriterAdapter(
        boost::shared_ptr<ParallelWriter<CELL_TYPE> > writer,
        const std::size_t firstStep,
        const std::size_t lastStep,
        bool lastCall) :
        writer(writer),
        firstNanoStep(firstStep * NANO_STEPS),
        lastNanoStep(lastStep   * NANO_STEPS),
        stride(writer->getPeriod() * NANO_STEPS),
        lastCall(lastCall)
    {
        pushRequest(firstNanoStep);
        pushRequest(lastNanoStep);
    }

    virtual void setRegion(const Region<GRID_TYPE::DIM>& region)
    {
        writer->setRegion(region);
    }

    virtual void put(
        const GRID_TYPE& grid,
        const Region<GRID_TYPE::DIM>& validRegion,
        const Coord<GRID_TYPE::DIM>& globalGridDimensions,
        const std::size_t nanoStep,
        const std::size_t rank)

    {
        if (!checkNanoStepPut(nanoStep)) {
            return;
        }

        WriterEvent event = WRITER_STEP_FINISHED;
        if (nanoStep == firstNanoStep) {
            event = WRITER_INITIALIZED;
        }
        if (nanoStep == lastNanoStep) {
            event = WRITER_ALL_DONE;
        }
        if (nanoStep > lastNanoStep) {
            return;
        }

        writer->stepFinished(
            grid,
            validRegion,
            globalGridDimensions,
            nanoStep / NANO_STEPS,
            event,
            rank,
            lastCall);
        erase_min(requestedNanoSteps);
        std::size_t nextNanoStep = nanoStep + stride;
        pushRequest(nextNanoStep);
    }

private:
    boost::shared_ptr<ParallelWriter<CELL_TYPE> > writer;
    std::size_t firstNanoStep;
    std::size_t lastNanoStep;
    std::size_t stride;
    bool lastCall;
};

}

#endif
