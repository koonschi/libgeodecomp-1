// vim: noai:ts=4:sw=4:expandtab
#ifndef LIBGEODECOMP_IO_VARSTEPINITIALIZERPROXY_H
#define LIBGEODECOMP_IO_VARSTEPINITIALIZERPROXY_H

#include <memory>
#include <libgeodecomp/io/initializer.h>
#include <libgeodecomp/config.h>
#include <libgeodecomp/io/clonableinitializer.h>

#ifdef LIBGEODECOMP_WITH_CPP14

namespace LibGeoDecomp {

/**
 * This class wraps another initializer but overrides its max steps
 * setting. This way a simulation can be run piece-wise.
 */
template<typename CELL>
class VarStepInitializerProxy : public ClonableInitializer<CELL>
{
public:
    friend class SimulationFactoryWithoutCudaTest;
    friend class SimulationFactoryWithCudaTest;

    typedef typename Initializer<CELL>::Topology Topology;
    const static int DIM = Topology::DIM;

    VarStepInitializerProxy(Initializer<CELL> *proxyObj) :
        ClonableInitializer<CELL>(),
        proxyObj(boost::shared_ptr<Initializer<CELL> >(proxyObj)),
        newMaxSteps(proxyObj->maxSteps())
    {}

    /**
     * We need this reset the number of steps in order to let
     * simulators run unto completion.
     */
    void resetMaxSteps()
    {
        newMaxSteps = proxyObj->maxSteps() - proxyObj->startStep();
    }

    /**
     * change the maxSteps to a new value
     */
    void setMaxSteps(unsigned steps) {
        newMaxSteps = steps;
    }

    //------------------- inherited functions from Initializer ------------------
    virtual void grid(GridBase<CELL,DIM> *target) override
    {
        proxyObj->grid(target);
    }

    virtual Coord<DIM> gridDimensions() const override
    {
        return proxyObj->gridDimensions();
    }

    virtual CoordBox<DIM> gridBox() override
    {
        return proxyObj->gridBox();
    }

    virtual unsigned startStep() const override
    {
        return proxyObj->startStep();
    }

    /**
     * This function return the step when the simulation
     * will be finished (startStep + getMaxSteps())
     */
    virtual unsigned maxSteps() const override
    {
        return proxyObj->startStep() + newMaxSteps;
    }

    virtual boost::shared_ptr<Adjacency> getAdjacency() const override
    {
        return proxyObj->getAdjacency();
    }

    //--------------- inherited functions from Clonableinitializer --------------
    virtual ClonableInitializer<CELL> *clone() const override
    {
        return new VarStepInitializerProxy<CELL>(*this);
    }

private:
    VarStepInitializerProxy(VarStepInitializerProxy<CELL>* o) :
        proxyObj(o->proxyObj),
        newMaxSteps(o->newMaxSteps)
    {}

    boost::shared_ptr<Initializer<CELL> > proxyObj;
    unsigned newMaxSteps;
};

}

#endif

#endif

