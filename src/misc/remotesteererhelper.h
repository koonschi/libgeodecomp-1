#ifndef LIBGEODECOMP_MISC_REMOTESTEERERHELPER_H
#define LIBGEODECOMP_MISC_REMOTESTEERERHELPER_H

#include <libgeodecomp/io/steererdata.h>
#include <libgeodecomp/misc/commandserver.h>
#include <libgeodecomp/misc/dataaccessor.h>

#include <sstream>
#include <mpi.h>

namespace LibGeoDecomp {

namespace RemoteSteererHelper {

/*
 * processes without direct access to the CommandServer Session buffer
 * their messages and root will collect and send them to the client
 */
// fixme: move to namespace or dedicated file
class MessageBuffer
{
public:
    /*
     *
     */
    MessageBuffer(MPI::Intracomm& comm, CommandServer::Session *session) :
        comm(comm),
        session(session)
    {}

    /*
     *
     */
    void sendMessage(std::string msg)
    {
        if (comm.Get_rank() == 0) {
            session->sendMessage("thread 0: " + msg);
        }
        else {
            msgBuffer.push_back(msg);
        }
    }

    void collectMessages()
    {
        int vectorSize = 0;
        int stringSize = 0;
        int msgCount[comm.Get_size()];
        if (comm.Get_rank() == 0) {
            comm.Gather(&vectorSize, 1, MPI::INT, msgCount, 1, MPI::INT, 0);

            // fixme: early return/continue
            for (int i = 1; i < comm.Get_size(); ++i) {
                if (msgCount[i] > 0) {
                    for (int j = 0; j < msgCount[i]; ++j) {
                        comm.Recv(&stringSize, 1, MPI::INT, i, 1);
                        char charBuffer[stringSize + 1];
                        charBuffer[stringSize] = '\0';
                        comm.Recv(charBuffer, stringSize, MPI::CHAR, i, 2);
                        std::string thread = "thread ";
                        std::stringstream ss;
                        ss << i;
                        thread += ss.str();
                        thread += ": ";
                        session->sendMessage(thread + std::string(charBuffer));
                    }
                }
            }
        }
        else {
            // fixme: decompose
            vectorSize = msgBuffer.size();
            comm.Gather(&vectorSize, 1, MPI::INT, msgCount, 1, MPI::INT, 0);

            if (vectorSize > 0) {
                for (int i = 0; i < vectorSize; ++i) {
                    stringSize = msgBuffer.at(i).size();
                    comm.Send(&stringSize, 1, MPI::INT, 0, 1);
                    char* tmp = const_cast<char*>(msgBuffer.at(i).c_str());
                    comm.Send(tmp, stringSize, MPI::CHAR, 0, 2);
                }
                msgBuffer.clear();
            }
        }
    }

private:
    std::vector<std::string> msgBuffer;
    MPI::Intracomm& comm;
    CommandServer::Session *session;
};

static double mystrtod(const char *ptr)
{
    errno = 0;
    double val = 0;
    char *endptr;
    val = strtod(ptr, &endptr);
    if ((errno != 0) || (ptr == endptr)) {
        throw(std::invalid_argument("bad parameter"));
    }
    return val;
}

static float mystrtof(const char *ptr)
{
    errno = 0;
    float val = 0;
    char *endptr;
    val = strtof(ptr, &endptr);
    if ((errno != 0) || (ptr == endptr)) {
        throw(std::invalid_argument("bad parameter"));
    }
    return val;
}

static long mystrtol(const char *ptr)
{
    errno = 0;
    long val = 0;
    char *endptr;
    val = strtol(ptr, &endptr, 10);
    if ((errno != 0) || (ptr == endptr)) {
        throw(std::invalid_argument("bad parameter"));
    }
    return val;
}

// fixme: replace this with library code
static int mystrtoi(const char *ptr)
{
    errno = 0;
    long val = 0;
    char *endptr;
    val = strtol(ptr, &endptr, 10);
    if ((errno != 0) || (val < INT_MIN) || (val > INT_MAX) || (ptr == endptr)) {
        throw(std::invalid_argument("bad parameter"));
    }
    return static_cast<int>(val);
}

template<typename CELL_TYPE>
static void executeGetRequests(
    typename Steerer<CELL_TYPE>::GridType*,
    const Region<Steerer<CELL_TYPE>::Topology::DIM>&,
    const unsigned&,
    CommandServer::Session*,
    void *);

template<typename CELL_TYPE>
static void executeSetRequests(
    typename Steerer<CELL_TYPE>::GridType*,
    const Region<Steerer<CELL_TYPE>::Topology::DIM>&,
    const unsigned&,
    CommandServer::Session*,
    void *);

// fixme: move this to dedicated file
template<typename CELL_TYPE, typename DATATYPE>
class SteererControl
{
  public:
    virtual void operator()(
        typename Steerer<CELL_TYPE>::GridType*,
        const Region<Steerer<CELL_TYPE>::Topology::DIM>&,
        const unsigned&, MessageBuffer*, void*,
        const MPI::Intracomm&, bool) = 0;
};

template<typename CELL_TYPE, typename DATATYPE>
class ExtendedSteererControlStub : SteererControl<CELL_TYPE, DATATYPE>
{
public:
    void operator()(
        typename Steerer<CELL_TYPE>::GridType *grid,
        const Region<Steerer<CELL_TYPE>::Topology::DIM>& validRegion,
        const unsigned& step,
        MessageBuffer *session,
        void *data,
        const MPI::Intracomm& comm,
        bool changed = false)
    {
        // do nothing, it's a stub
    }
};

template<typename CELL_TYPE,
         typename DATATYPE=SteererData<CELL_TYPE>,
         typename EXTENDED=ExtendedSteererControlStub<CELL_TYPE, DATATYPE> >
class DefaultSteererControl : SteererControl<CELL_TYPE, DATATYPE>
{
public:
    void operator()(
        typename Steerer<CELL_TYPE>::GridType *grid,
        const Region<Steerer<CELL_TYPE>::Topology::DIM>& validRegion,
        const unsigned& step,
        MessageBuffer *session,
        void *data,
        const MPI::Intracomm& comm,
        bool changed = false)
    {
        // fixme: bad variable name
        DATATYPE *sdata = (DATATYPE*) data;

        bool newcommands = false;
        if (comm.Get_rank() == 0) {
            if (sdata->finishMutex.try_lock()) {
                newcommands = true;
            }
        }

        if (comm.Get_size() > 1) {
            comm.Bcast(&newcommands, 1, MPI::BOOL, 0);
        }

        if (newcommands) {
            if (comm.Get_size() > 1) {
                sdata->broadcastSteererData();
            }

            EXTENDED()(grid, validRegion, step, session, data, comm, true);

            if (sdata->setX.size() > 0) {
                executeSetRequests<CELL_TYPE>(
                        grid, validRegion, step, session, data);
            }

            if (sdata->getX.size() > 0) {
                executeGetRequests<CELL_TYPE>(
                        grid, validRegion, step, session, data);
            }

            if (comm.Get_rank() == 0) {
                sdata->waitMutex.unlock();
            }
        }
    }
};


// fixme: move to dedicated file
/*
 * wrapper class for requests to cell variables and
 * partial specialized for dimensions
 */
template<typename T, typename CELL_TYPE, int DIM>
class Request;

template<typename T, typename CELL_TYPE>
class Request<T, CELL_TYPE,2> 
{
public:
    static bool validateCoords(
        typename Steerer<CELL_TYPE>::GridType *grid,
        const Region<Steerer<CELL_TYPE>::Topology::DIM>& validRegion,
        const int x, const int y, const int z)
    {
        return validRegion.boundingBox().inBounds(Coord<2>(x, y));
    }

    /*
     *
     */
    static int associateAccessor (
        DataAccessor<CELL_TYPE>** dataAccessors,
        int num, std::string name)
    {
        for (int i = 0; i < num; ++i) {
            if ((dataAccessors[i]->getName()).compare(name) == 0) {
                return i;
            }
        }

        return -1;
    }

    /*
     *
     */
    static T valueRequest(
        DataAccessor<CELL_TYPE>* dataAccessor,
        typename Steerer<CELL_TYPE>::GridType *grid,
        const int x,
        const int y,
        const int z)
    {
        T value;
        value = 0;
        dataAccessor->getFunction(grid->at(Coord<2>(x, y)),
                                  reinterpret_cast<void*>(&value));
        return value;
    }

    static void mutationRequest(
        DataAccessor<CELL_TYPE>* dataAccessor,
        typename Steerer<CELL_TYPE>::GridType *grid,
        const int x,
        const int y,
        const int z,
        T value)
    {
        dataAccessor->setFunction(
            &(grid->at(Coord<2>(x, y))),
            reinterpret_cast<void*>(&value));
    }
};

template<typename T, typename CELL_TYPE>
class Request<T, CELL_TYPE, 3>
{
  public:
    static bool validateCoords(
        typename Steerer<CELL_TYPE>::GridType *grid,
        const Region<Steerer<CELL_TYPE>::Topology::DIM>& validRegion,
        const int x,
        const int y,
        const int z)
    {
        return validRegion.boundingBox().inBounds(Coord<3>(x, y, z));
    }

    static int associateAccessor(
        DataAccessor<CELL_TYPE> **dataAccessors,
        int num,
        std::string name)
    {
        for (int i = 0; i < num; ++i) {
            if ((dataAccessors[i]->getName()).compare(name) == 0) {
                return i;
            }
        }

        return -1;
    }

    static T valueRequest(
        DataAccessor<CELL_TYPE>* dataAccessor,
        typename Steerer<CELL_TYPE>::GridType *grid,
        const int x,
        const int y,
        const int z)
    {
        T value;
        value = 0;
        dataAccessor->getFunction(
            grid->at(Coord<3>(x, y, z)),
            reinterpret_cast<void*>(&value));
        return value;
    }

    static void mutationRequest(
        DataAccessor<CELL_TYPE>* dataAccessor,
        typename Steerer<CELL_TYPE>::GridType *grid,
        const int x,
        const int y,
        const int z,
        T value)
    {
        dataAccessor->setFunction(
            &(grid->at(Coord<3>(x, y, z))),
            reinterpret_cast<void*>(&value));
    }
};

template<typename CELL_TYPE>
static void executeGetRequests(
    typename Steerer<CELL_TYPE>::GridType *grid,
    const Region<Steerer<CELL_TYPE>::Topology::DIM>& validRegion,
    const unsigned& step,
    MessageBuffer *session,
    void *data)
{
    static const int DIM = Steerer<CELL_TYPE>::Topology::DIM;
    SteererData<CELL_TYPE>* sdata = (SteererData<CELL_TYPE>*) data;

    // fixme: function too long
    for (int j = 0; j < sdata->getX.size(); ++j) {
        if ((DIM == 3) && (sdata->getZ[j] < 0)) {
            session->sendMessage("3 dimensional coords needed\n");
            continue;
        }
        if ((DIM == 2) && (sdata->getZ[j] > 0)) {
            session->sendMessage("2 dimensional coords needed\n");
            continue;
        }
        if (!Request<int, CELL_TYPE, DIM>::validateCoords(
                grid, validRegion, sdata->getX[j],
                sdata->getY[j], sdata->getZ[j])) {
            std::string msg = "no valid coords: ";
            msg += boost::to_string(sdata->getX[j]) + " ";
            msg += boost::to_string(sdata->getY[j]);
            if (DIM == 3) {
                msg += " " + boost::to_string(sdata->getZ[j]);
            }
            msg += "\n";
            session->sendMessage(msg);
            continue;
        }

        for (int i = 0; i < sdata->numVars; ++i) {
            std::string output = sdata->dataAccessors[i]->getName();
            output += "[" + boost::to_string(sdata->getX[j]) + "][";
            output += boost::to_string(sdata->getY[j]) + "]";
            if (grid->DIM == 3) {
                output += "[";
                output += boost::to_string(sdata->getZ[j]) + "]";
            }
            output += " = ";

            if (strcmp("DOUBLE", sdata->dataAccessors[i]->
                                 getType().c_str()) == 0) {
                double value = Request<double, CELL_TYPE, DIM>::
                        valueRequest(
                            sdata->dataAccessors[i], grid,
                            sdata->getX[j], sdata->getY[j],
                            sdata->getZ[j]);
                output += boost::to_string(value);
            } else if (strcmp("INT", sdata->dataAccessors[i]->
                                   getType().c_str()) == 0) {
                int value = Request<int, CELL_TYPE, DIM>::
                        valueRequest(
                            sdata->dataAccessors[i], grid,
                            sdata->getX[j], sdata->getY[j],
                            sdata->getZ[j]);
                output += boost::to_string(value);
            } else if (strcmp("FLOAT", sdata->dataAccessors[i]->
                                     getType().c_str()) == 0) {
                float value = Request<float, CELL_TYPE, DIM>::
                        valueRequest(
                            sdata->dataAccessors[i], grid,
                            sdata->getX[j], sdata->getY[j],
                            sdata->getZ[j]);
                output += boost::to_string(value);
            } else if (strcmp("CHAR", sdata->dataAccessors[i]->
                                    getType().c_str()) == 0) {
                char value = Request<char, CELL_TYPE, DIM>::
                        valueRequest(
                            sdata->dataAccessors[i], grid,
                            sdata->getX[j], sdata->getY[j],
                            sdata->getZ[j]);
                output += boost::to_string(value);
            } else if (strcmp("LONG", sdata->dataAccessors[i]->
                                    getType().c_str()) == 0) {
                long value = Request<long, CELL_TYPE, DIM>::
                        valueRequest(
                            sdata->dataAccessors[i], grid,
                            sdata->getX[j], sdata->getY[j],
                            sdata->getZ[j]);
                output += boost::to_string(value);
            }
            output += "\n";
            session->sendMessage(output);
        }
    }
    sdata->getX.clear();
    sdata->getY.clear();
    sdata->getZ.clear();
}

template<typename CELL_TYPE>
static void executeSetRequests(
    typename Steerer<CELL_TYPE>::GridType *grid,
    const Region<Steerer<CELL_TYPE>::Topology::DIM>& validRegion,
    const unsigned& step,
    MessageBuffer* session,
    void *data) 
{
    static const int DIM = Steerer<CELL_TYPE>::Topology::DIM;
    SteererData<CELL_TYPE>* sdata = (SteererData<CELL_TYPE>*) data;

    // fixme: function too long
    for (int j = 0; j < sdata->setX.size(); ++j) {
        int accessor;

        if ((DIM == 3) && (sdata->setZ[j] < 0)) {
            session->sendMessage("3 dimensional coords needed\n");
            continue;
        }
        if (!Request<int, CELL_TYPE, DIM>::validateCoords(
                grid, validRegion, sdata->setX[j],
                sdata->setY[j], sdata->setZ[j])) {
            std::string msg = "no valid coords: ";
            msg += boost::to_string(sdata->setX[j]) + " ";
            msg += boost::to_string(sdata->setY[j]);
            if (DIM == 3) {
                msg += " " + boost::to_string(sdata->setZ[j]);
            }
            msg += "\n";
            session->sendMessage(msg);
            continue;
        }
        accessor = Request<int, CELL_TYPE, DIM>::associateAccessor(
               sdata->dataAccessors, sdata->numVars, sdata->var[j]);
        if (accessor < 0) {
            std::string msg = "no valid variable: ";
            msg += sdata->var[j] + "\n";
            session->sendMessage(msg);
            continue;
        }
        std::string output = sdata->dataAccessors[accessor]->getName();
        output += " set [" + boost::to_string(sdata->setX[j]) + "][";
        output += boost::to_string(sdata->setY[j]) + "]";
        if (grid->DIM == 3) {
            output += "[";
            output += boost::to_string(sdata->setZ[j]) + "]";
        }
        output += " to ";

        if (strcmp("DOUBLE", sdata->dataAccessors[accessor]->
                             getType().c_str()) == 0) {
            double value;
            try {
                value = mystrtod(sdata->val[j].c_str());
            }
            catch (std::exception& e) {
                std::string msg = "bad value: " + sdata->val[j] + "\n";
                session->sendMessage(msg);
                continue;
            }
            Request<double, CELL_TYPE, DIM>::
                    mutationRequest(
                        sdata->dataAccessors[accessor], grid,
                        sdata->setX[j], sdata->setY[j],
                        sdata->setZ[j], value);
            session->sendMessage(output + boost::to_string(value) + "\n");
        } else if (strcmp("INT", sdata->dataAccessors[accessor]->
                               getType().c_str()) == 0) {
            int value;
            try {
                value = mystrtoi(sdata->val[j].c_str());
            }
            catch (std::exception& e) {
                std::string msg = "bad value: " + sdata->val[j] + "\n";
                session->sendMessage(msg);
                continue;
            }
            Request<int, CELL_TYPE, DIM>::
                    mutationRequest(
                        sdata->dataAccessors[accessor], grid,
                        sdata->setX[j], sdata->setY[j],
                        sdata->setZ[j], value);
            session->sendMessage(output + boost::to_string(value) + "\n");
        } else if (strcmp("FLOAT", sdata->dataAccessors[accessor]->
                                 getType().c_str()) == 0) {
            float value;
            try {
                value = mystrtof(sdata->val[j].c_str());
            }
            catch (std::exception& e) {
                std::string msg = "bad value: " + sdata->val[j] + "\n";
                session->sendMessage(msg);
                continue;
            }
            Request<float, CELL_TYPE, DIM>::
                    mutationRequest(
                        sdata->dataAccessors[accessor], grid,
                        sdata->setX[j], sdata->setY[j],
                        sdata->setZ[j], value);
            session->sendMessage(output + boost::to_string(value) + "\n");
        } else if (strcmp("CHAR", sdata->dataAccessors[accessor]->
                                getType().c_str()) == 0) {
            char value;
            value = sdata->val[j][0];
            Request<char, CELL_TYPE, DIM>::
                    mutationRequest(
                        sdata->dataAccessors[accessor], grid,
                        sdata->setX[j], sdata->setY[j],
                        sdata->setZ[j], value);
            session->sendMessage(output + boost::to_string(value) + "\n");
        } else if (strcmp("LONG", sdata->dataAccessors[accessor]->
                                getType().c_str()) == 0) {
            long value;
            try {
                value = mystrtol(sdata->val[j].c_str());
            }
            catch (std::exception& e) {
                std::string msg = "bad value: " + sdata->val[j] + "\n";
                session->sendMessage(msg);
                continue;
            }
            Request<long, CELL_TYPE, DIM>::
                    mutationRequest(
                        sdata->dataAccessors[accessor], grid,
                        sdata->setX[j], sdata->setY[j],
                        sdata->setZ[j], value);
            session->sendMessage(output + boost::to_string(value) + "\n");
        }
    }
    sdata->setX.clear();
    sdata->setY.clear();
    sdata->setZ.clear();
    sdata->val.clear();
    sdata->var.clear();
}

}

}

#endif
