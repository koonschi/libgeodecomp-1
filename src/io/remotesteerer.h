#include <libgeodecomp/config.h>
#ifdef LIBGEODECOMP_FEATURE_MPI
#ifndef LIBGEODECOMP_IO_REMOTESTEERER_H
#define LIBGEODECOMP_IO_REMOTESTEERER_H

#include <libgeodecomp/io/steerer.h>
#include <libgeodecomp/io/remotesteerer/commandserver.h>
#include <libgeodecomp/io/remotesteerer/remotesteererhelper.h>
#include <libgeodecomp/mpilayer/typemaps.h>
#include <mpi.h>

namespace LibGeoDecomp {

// fixme: remove this?
using namespace RemoteSteererHelpers;

/**
 * The RemoteSteerer allows the user to control a parallel simulation
 * from a single network connection.
 */
template<typename CELL_TYPE,
         typename STEERER_DATA_TYPE=SteererData<CELL_TYPE>,
         typename CONTROL=DefaultSteererControl<CELL_TYPE> >
class RemoteSteerer : public Steerer<CELL_TYPE>
{
public:
    typedef typename Steerer<CELL_TYPE>::Topology Topology;
    typedef typename Steerer<CELL_TYPE>::GridType GridType;

    RemoteSteerer(
        const unsigned& period,
        int port,
        CommandServer::FunctionMap commandMap = getDefaultMap(),
        STEERER_DATA_TYPE *steererData = 0,
        const MPI::Intracomm& comm = MPI::COMM_WORLD) :
        Steerer<CELL_TYPE>(period),
        server(0),
        steererData(steererData),
        comm(comm)
    {
        // fixme: use mpilayer here?
        if (comm.Get_rank() != 0) {
            return;
        }

        // only listen on rank 0:
        server = new CommandServer(port, commandMap, steererData);
    }

    virtual ~RemoteSteerer()
    {
        // fixme: avoid delete. do we really need defaultData/Map?
        delete server;
    }

    static CommandServer::FunctionMap getDefaultMap()
    {
        CommandServer::FunctionMap defaultMap;
        defaultMap["help"] = helpFunction;
        defaultMap["get"] = getFunction;
        defaultMap["set"] = setFunction;
        defaultMap["finish"] = finishFunction;

        return defaultMap;
    }

    virtual void nextStep(
            GridType *grid,
            const Region<Topology::DIM>& validRegion,
            unsigned step)
    {
        CommandServerProxy proxy(comm, server);

        // fixme: get rid of CONTROL?
        CONTROL()(grid, validRegion, step, &proxy, steererData, comm);
        // fixme: why not do this always?
        if (comm.Get_size() > 1) {
            proxy.collectMessages();
        }
    }

    static void helpFunction(
        std::vector<std::string> stringVec,
        CommandServer *commandServer,
        void *data)
    {
        CommandServer::FunctionMap commandMap = commandServer->getMap();

        for (CommandServer::FunctionMap::const_iterator i = commandMap.begin();
             i != commandMap.end();
             ++i) {
            std::string command = i->first;
            if (command.compare("help") != 0) {
                commandServer->sendMessage(command + ":\n");
                std::vector<std::string> parameter;
                parameter.push_back(i->first);
                parameter.push_back("help");
                i->second(parameter, commandServer, data);
            }
        }
    }

    // fixme: use functors for callbacks
    // fixme: if these are issued by clients, why not use member functions?
    static void getFunction(std::vector<std::string> stringVec,
                            CommandServer *server,
                            void *data)
    {
        STEERER_DATA_TYPE *sdata = (STEERER_DATA_TYPE*) data;
        int x = 0;
        int y = 0;
        int z = -1;
        std::string helpMsg = "    Usage: get <x> <y> [<z>]\n";
        helpMsg += "          adds a get action to the list\n";
        helpMsg += "          <x>, <y> and <z> must be integers\n";
        helpMsg += "          for execution use finish\n";
        try {
            if (stringVec.at(1).compare("help") == 0) {
                server->sendMessage(helpMsg);
                return;
            }
            x = mystrtoi(stringVec.at(1).c_str());
            y = mystrtoi(stringVec.at(2).c_str());
            if (stringVec.size() > 3) {
                z = mystrtoi(stringVec.at(3).c_str());
            }
        }
        catch (std::exception& e) {
            server->sendMessage(helpMsg);
            return;
        }
        sdata->getX.push_back(x);
        sdata->getY.push_back(y);
        sdata->getZ.push_back(z);
    }

    static void setFunction(std::vector<std::string> stringVec,
                            CommandServer *server,
                            void *data)
    {
        STEERER_DATA_TYPE *sdata = (STEERER_DATA_TYPE*) data;
        int x = 0;
        int y = 0;
        int z = -1;
        std::string value;
        std::string var;
        std::string helpMsg = "    Usage: set <variable> <value> <x> <y> [<z>]\n";
        helpMsg += "          add a set action to the list\n";
        helpMsg += "          <x>, <y> and <z> must be integers\n";
        helpMsg += "          for execution use finish\n";
        try {
            if (stringVec.at(1).compare("help") == 0) {
                server->sendMessage(helpMsg);
                return;
            }
            var = stringVec.at(1);
            value = stringVec.at(2);
            x = mystrtoi(stringVec.at(3).c_str());
            y = mystrtoi(stringVec.at(4).c_str());
            if (stringVec.size() > 5) {
                z = mystrtoi(stringVec.at(5).c_str());
            }
        }
        catch (std::exception& e) {
            server->sendMessage(helpMsg);
            return;
        }
        sdata->setX.push_back(x);
        sdata->setY.push_back(y);
        sdata->setZ.push_back(z);
        sdata->val.push_back(value);
        sdata->var.push_back(var);
    }

    static void finishFunction(std::vector<std::string> stringVec,
                            CommandServer *server,
                            void *data)
    {
        STEERER_DATA_TYPE *sdata = (STEERER_DATA_TYPE*) data;
        std::string helpMsg = "    Usage: finish\n";
        helpMsg += "          sets lock and waits until a step is finished\n";
        helpMsg += "          has to be used to start default set and get operations\n";
        if (stringVec.size() > 1) {
            server->sendMessage(helpMsg);
            return;
        }
        server->sendMessage("waiting until next step finished ...\n");
        sdata->finishMutex.unlock();
        sdata->waitMutex.lock();
    }

private:
    CommandServer *server;
    // fixme: use shared_ptr here and in command server
    STEERER_DATA_TYPE *steererData;
    MPI::Intracomm comm;

};

}

#endif
#endif
