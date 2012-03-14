#include <cxxtest/TestSuite.h>

#include <libgeodecomp/io/testinitializer.h>
#include <libgeodecomp/misc/testhelper.h>
#include <libgeodecomp/parallelization/hiparsimulator/patchlink.h>

using namespace LibGeoDecomp; 
using namespace HiParSimulator; 

namespace LibGeoDecomp {
namespace HiParSimulator {

class PatchLinkTest : public CxxTest::TestSuite
{
public:
    typedef Grid<int> GridType;
    typedef PatchLink<GridType>::Accepter MyPatchAccepter;
    typedef PatchLink<GridType>::Provider MyPatchProvider;

    void setUp()
    {
        region1.clear();
        region1 << Streak<2>(Coord<2>(2, 2), 4);
        region1 << Streak<2>(Coord<2>(2, 3), 5);
        region2.clear();
        region2 << Streak<2>(Coord<2>(0, 0), 6);
        region2 << Streak<2>(Coord<2>(4, 1), 5);

        zeroGrid  = GridType(Coord<2>(7, 5), 0);
        boundingBox.clear();
        boundingBox << CoordBox<2>(Coord<2>(0, 0), Coord<2>(7, 5));
            
        sendGrid1 = markGrid(region1, mpiLayer.rank());
        sendGrid2 = markGrid(region2, mpiLayer.rank());

        tag = 69;
    }

    void tearDown()
    {
        acc.reset();
        pro.reset();
    }

    void testBasic() 
    {
        int nanoStep = 0;
        for (int sender = 0; sender < mpiLayer.size(); ++sender) {
            for (int receiver = 0; receiver < mpiLayer.size(); ++receiver) {
                if (sender != receiver) {
                    Region<2>& region  = sender % 2 ? region2 : region1;
                    GridType& sendGrid = sender % 2 ? sendGrid2 : sendGrid1;
                    long nanoStep = sender * receiver + 4711;
                    
                    if (sender == mpiLayer.rank()) {
                        acc.reset(new MyPatchAccepter(
                                      region,
                                      receiver,
                                      tag,
                                      MPI::INT));

                        acc->pushRequest(nanoStep);
                        acc->put(sendGrid, boundingBox, nanoStep);
                        acc->wait();
                    }

                    if (receiver == mpiLayer.rank()) {
                        pro.reset(new MyPatchProvider(
                                      region,
                                      sender,
                                      tag,
                                      MPI::INT));

                        expected = markGrid(region, sender);
                        actual = zeroGrid;

                        pro->recv(nanoStep);
                        pro->wait();
                        pro->get(&actual, boundingBox, nanoStep);
                        TS_ASSERT_EQUALS(actual, expected);
                    }
                }

                ++nanoStep;
            }
        }
    }

    void testMultiple() 
    {
        SuperVector<MyPatchAccepter> accepters;
        SuperVector<MyPatchProvider> providers;
        int stride = 4;
        int maxNanoSteps = 31;

        for (int i = 0; i < mpiLayer.size(); ++i) {
            if (i != mpiLayer.rank()) {
                accepters << MyPatchAccepter(
                    region1,
                    i,
                    genTag(mpiLayer.rank(), i),
                    MPI::INT);
                
                providers << MyPatchProvider(
                    region1,
                    i,
                    genTag(i, mpiLayer.rank()),
                    MPI::INT);
            }
        }

        for (int i = 0; i < mpiLayer.size() - 1; ++i) {
            accepters[i].charge(0, maxNanoSteps, stride);
            providers[i].charge(0, maxNanoSteps, stride);
        }

        for (int nanoStep = 0; nanoStep < maxNanoSteps; nanoStep += stride) {
            GridType mySendGrid = markGrid(region1, mpiLayer.rank() * 10000 + nanoStep * 100);
        
            for (int i = 0; i < mpiLayer.size() - 1; ++i) 
                accepters[i].put(mySendGrid, boundingBox, nanoStep);

            for (int i = 0; i < mpiLayer.size() - 1; ++i) {
                int senderRank = i >= mpiLayer.rank() ? i + 1 : i;
                GridType expected = markGrid(region1, senderRank * 10000 + nanoStep * 100);
                GridType actual = zeroGrid;
                providers[i].get(&actual, boundingBox, nanoStep);

                TS_ASSERT_EQUALS(actual, expected);
            }
        }
    }

    void testMultiple2() 
    {
        SuperVector<MyPatchAccepter> accepters;
        SuperVector<MyPatchProvider> providers;
        int stride = 4;
        int maxNanoSteps = 100;

        for (int i = 0; i < mpiLayer.size(); ++i) {
            if (i != mpiLayer.rank()) {
                accepters << MyPatchAccepter(
                    region1,
                    i,
                    genTag(mpiLayer.rank(), i),
                    MPI::INT);
                
                providers << MyPatchProvider(
                    region1,
                    i,
                    genTag(i, mpiLayer.rank()),
                    MPI::INT);
            }
        }

        for (int i = 0; i < mpiLayer.size() - 1; ++i) {
            accepters[i].charge(0, PatchLink<GridType>::ENDLESS, stride);
            providers[i].charge(0, PatchLink<GridType>::ENDLESS, stride);
        }

        for (int nanoStep = 0; nanoStep < maxNanoSteps; nanoStep += stride) {
            GridType mySendGrid = markGrid(region1, mpiLayer.rank() * 10000 + nanoStep * 100);
        
            for (int i = 0; i < mpiLayer.size() - 1; ++i) 
                accepters[i].put(mySendGrid, boundingBox, nanoStep);

            for (int i = 0; i < mpiLayer.size() - 1; ++i) {
                int senderRank = i >= mpiLayer.rank() ? i + 1 : i;
                GridType expected = markGrid(region1, senderRank * 10000 + nanoStep * 100);
                GridType actual = zeroGrid;
                providers[i].get(&actual, boundingBox, nanoStep);

                TS_ASSERT_EQUALS(actual, expected);
            }
        }

        for (int i = 0; i < mpiLayer.size() - 1; ++i) {
            accepters[i].cancel();
            providers[i].cancel();
        }
    }

private:
    int tag;
    MPILayer mpiLayer;

    GridType zeroGrid;
    GridType sendGrid1;
    GridType sendGrid2;
    GridType expected;
    GridType actual;

    Region<2> boundingBox;
    Region<2> region1;
    Region<2> region2;

    boost::shared_ptr<MyPatchAccepter> acc;
    boost::shared_ptr<MyPatchProvider> pro;

    GridType markGrid(const Region<2>& region, const int& id)
    {
        GridType ret = zeroGrid;
        for (Region<2>::Iterator i = region.begin(); i != region.end(); ++i)
            ret[*i] = id + i->y() * 10 + i->x();
        return ret;
    }

    int genTag(const int& from, const int& to) {
        return 100 + from * 10 + to;
    }
};

}
}
