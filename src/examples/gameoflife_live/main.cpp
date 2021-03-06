#include <libgeodecomp/communication/mpilayer.h>
#include <libgeodecomp/communication/typemaps.h>
#include <libgeodecomp/io/bovwriter.h>
#include <libgeodecomp/io/simpleinitializer.h>
#include <libgeodecomp/io/ppmwriter.h>
#include <libgeodecomp/io/remotesteerer.h>
#include <libgeodecomp/io/simplecellplotter.h>
#include <libgeodecomp/io/simpleinitializer.h>
#include <libgeodecomp/io/tracingwriter.h>
#include <libgeodecomp/io/visitwriter.h>
#include <libgeodecomp/io/remotesteerer/commandserver.h>
#include <libgeodecomp/loadbalancer/oozebalancer.h>
#include <libgeodecomp/loadbalancer/tracingbalancer.h>
#include <libgeodecomp/parallelization/serialsimulator.h>
#include <libgeodecomp/parallelization/stripingsimulator.h>
#include <libgeodecomp/storage/image.h>
#include <boost/assign/std/vector.hpp>

using namespace boost::assign;
using namespace LibGeoDecomp;

class ConwayCell
{
public:
    explicit ConwayCell(bool alive = false) :
        alive(alive)
    {
        count = alive? 1 : 0;
    }

    int countLivingNeighbors(const CoordMap<ConwayCell>& neighborhood)
    {
        int ret = 0;
        for (int y = -1; y < 2; ++y) {
            for (int x = -1; x < 2; ++x) {
                ret += neighborhood[Coord<2>(x, y)].alive;
            }
        }
        ret -= neighborhood[Coord<2>(0, 0)].alive;
        return ret;
    }

    void update(const CoordMap<ConwayCell>& neighborhood, unsigned)
    {
        int livingNeighbors = countLivingNeighbors(neighborhood);
        alive = neighborhood[Coord<2>(0, 0)].alive;
        if (alive) {
            alive = (2 <= livingNeighbors) && (livingNeighbors <= 3);
        } else {
            alive = (livingNeighbors == 3);
        }
        if(alive)
            count += 1;
    }

    char alive;
    int count;
};

class CellInitializer : public SimpleInitializer<ConwayCell>
{
public:
    CellInitializer() : SimpleInitializer<ConwayCell>(Coord<2>(160, 90), 80000)
    {}

    virtual void grid(GridBase<ConwayCell, 2> *ret)
    {
        CoordBox<2> rect = ret->boundingBox();
        std::vector<Coord<2> > startCells;
        // start with a single glider...
        //          x
        //           x
        //         xxx
        startCells +=
            Coord<2>(11, 10),
            Coord<2>(12, 11),
            Coord<2>(10, 12), Coord<2>(11, 12), Coord<2>(12, 12);


        // ...add a Diehard pattern...
        //                x
        //          xx
        //           x   xxx
        startCells +=
            Coord<2>(55, 70), Coord<2>(56, 70), Coord<2>(56, 71),
            Coord<2>(60, 71), Coord<2>(61, 71), Coord<2>(62, 71),
            Coord<2>(61, 69);

        // ...and an Acorn pattern:
        //        x
        //          x
        //       xx  xxx
        startCells +=
            Coord<2>(111, 30),
            Coord<2>(113, 31),
            Coord<2>(110, 32), Coord<2>(111, 32),
            Coord<2>(113, 32), Coord<2>(114, 32), Coord<2>(115, 32);

        for (std::vector<Coord<2> >::iterator i = startCells.begin();
             i != startCells.end();
             ++i) {
            if (rect.inBounds(*i)) {
                ret->set(*i, ConwayCell(true));
            }
        }
    }
};

void runSimulation()
{
    int outputFrequency = 1;

    SerialSimulator<ConwayCell> sim(
        new CellInitializer());


    VisItWriter<ConwayCell> *visItWriter = new VisItWriter<ConwayCell>(
        "gameOfLife", outputFrequency, true);
    visItWriter->addVariable(&ConwayCell::alive, "alive");
    visItWriter->addVariable(&ConwayCell::count, "count");

    sim.addWriter(visItWriter);

    // /*
    //  * ---------------------------------------------
    //  * extend default remote steerer commands part 2
    //  * ---------------------------------------------
    //  */
    // CommandServer<ConwayCell>::FunctionMap functionMap = RemoteSteerer<ConwayCell>::getDefaultMap();
    // // fixme: reenable this
    // // functionMap["size"] = sizeFunction;

    // SteererData<ConwayCell> *myData = new SteererData<ConwayCell>();
    // myData->addVariable(new aliveDataAccessor());
    // myData->addVariable(new countDataAccessor());

    // Steerer<ConwayCell> *steerer =
    //     new RemoteSteerer<ConwayCell>(
    //         1,
    //         1234,
    //         functionMap,
    //         myData);
    // sim.addSteerer(steerer);

    sim.run();
}

int main(int argc, char *argv[])
{
    runSimulation();
    return 0;
}
