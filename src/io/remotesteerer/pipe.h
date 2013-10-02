#ifndef LIBGEODECOMP_IO_REMOTESTEERER_PIPE_H
#define LIBGEODECOMP_IO_REMOTESTEERER_PIPE_H

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <libgeodecomp/config.h>
#include <libgeodecomp/io/logger.h>
#include <libgeodecomp/misc/stringops.h>
#include <libgeodecomp/mpilayer/mpilayer.h>

namespace LibGeoDecomp {

namespace RemoteSteererHelpers {

/**
 * The purpose of the Pipe is to forward steering data between the
 * connection node and the execution nodes. This forwarding is done
 * asynchronously as Actions from the connection node run in a
 * seperate thread while Handlers are run by the RemoteSteerer, which
 * in turn is triggered by the Simulator. The Pipe also manages
 * scattering/gathering of the data via MPI, which is essentially a
 * 1-n operation. The external interface is thread-safe.
 */
class Pipe
{
public:
    friend class PipeTest;

#ifdef LIBGEODECOMP_FEATURE_MPI
    Pipe(
        unsigned root = 0,
        MPI_Comm communicator = MPI_COMM_WORLD) :
        mpiLayer(communicator),
        root(root)
    {}
#endif

    void addSteeringRequest(const std::string& request)
    {
        LOG(DBG, "Pipe::addSteeringRequest(" << request << ")");
        boost::lock_guard<boost::mutex> lock(mutex);
        steeringRequestsQueue << request;
    }

    void addSteeringFeedback(const std::string& feedback)
    {
        LOG(DBG, "Pipe::addSteeringFeedback(" << feedback << ")");
        boost::lock_guard<boost::mutex> lock(mutex);
        steeringFeedback << feedback;
        signal.notify_one();
    }

    StringVec retrieveSteeringRequests()
    {
        LOG(DBG, "Pipe::retrieveSteeringRequests()");
        StringVec requests;
        boost::lock_guard<boost::mutex> lock(mutex);
        std::swap(requests, steeringRequests);
        LOG(DBG, "  retrieveSteeringRequests yields " << requests.size());
        if (requests.size() > 0) {
            LOG(DBG, "  steeringRequests: " << requests);
        }
        return requests;
    }

    StringVec copySteeringRequestsQueue()
    {
        LOG(DBG, "Pipe::copySteeringRequestsQueue()");
        boost::lock_guard<boost::mutex> lock(mutex);
        StringVec requests = steeringRequestsQueue;
        return requests;
    }

    StringVec retrieveSteeringFeedback()
    {
        LOG(DBG, "Pipe::retrieveSteeringFeedback()");
        StringVec feedback;
        boost::lock_guard<boost::mutex> lock(mutex);
        std::swap(feedback, steeringFeedback);
        LOG(DBG, "  retrieveSteeringFeedback yields " << feedback.size());
        LOG(DBG, "  retrieveSteeringFeedback is " << feedback);
        return feedback;
    }

    StringVec copySteeringFeedback()
    {
        LOG(DBG, "Pipe::copySteeringFeedback()");
        boost::lock_guard<boost::mutex> lock(mutex);
        StringVec feedback = steeringFeedback;
        return feedback;
    }

    void sync()
    {
        LOG(DBG, "Pipe::sync()");
        boost::lock_guard<boost::mutex> lock(mutex);
#ifdef LIBGEODECOMP_FEATURE_MPI
        broadcastSteeringRequests();
        moveSteeringFeedbackToRoot();
#endif
    }

    void waitForFeedback(size_t lines = 1)
    {
        LOG(DBG, "Pipe::waitForFeedback(" << lines << ")");
        boost::unique_lock<boost::mutex> lock(mutex);

        while (steeringFeedback.size() < lines) {
            LOG(DBG, "  still waiting for feedback (" << steeringFeedback.size() << "/" << lines << ")\n");
            signal.wait(lock);
        }
        LOG(DBG, "  feedback acquired");
    }

private:
    boost::mutex mutex;
    boost::condition_variable signal;
    StringVec steeringRequestsQueue;
    StringVec steeringRequests;
    StringVec steeringFeedback;

#ifdef LIBGEODECOMP_FEATURE_MPI
    MPILayer mpiLayer;
    unsigned root;

    void broadcastSteeringRequests()
    {
        LOG(DBG, "Pipe::broadcastSteeringRequests()");

        int numRequests = mpiLayer.broadcast(steeringRequestsQueue.size(), root);
        if (mpiLayer.rank() != root) {
            steeringRequestsQueue.resize(numRequests);
        } else {
            LOG(DBG, "  steeringRequestsQueue: " << steeringRequestsQueue);
        }

        std::vector<int> requestSizes(numRequests);

        if (mpiLayer.rank() == root) {
            for (int i = 0; i < numRequests; ++i) {
                requestSizes[i] = steeringRequestsQueue[i].size();
            }
        }

        mpiLayer.broadcastVector(&requestSizes, root);
        if (mpiLayer.rank() != root) {
            for (int i = 0; i < numRequests; ++i) {
                steeringRequestsQueue[i].resize(requestSizes[i]);
            }
        }

        for (int i = 0; i < numRequests; ++i) {
            mpiLayer.broadcast(&steeringRequestsQueue[i][0], requestSizes[i], root);
        }
        append(steeringRequests, steeringRequestsQueue);
        steeringRequestsQueue.clear();
        LOG(DBG, "  steeringRequests: " << steeringRequests);
    }

    /**
     * Will move steering requests from the compute nodes to the root
     * where it can then be forwarded to the user.
     */
    void moveSteeringFeedbackToRoot()
    {
        // marshall all feedback in order to use scalable gather afterwards
        std::vector<char> localBuffer;
        std::vector<int> localLengths;
        for (StringVec::iterator i = steeringFeedback.begin();
             i != steeringFeedback.end();
             ++i) {
            localBuffer.insert(localBuffer.end(), i->begin(), i->end());
            localLengths << i->size();
        }
        LOG(DBG, "moveSteeringFeedbackToRoot: " << localBuffer << " " << localLengths);

        // how many strings are sent per node?
        std::vector<int> numFeedback = mpiLayer.gather((int)localLengths.size(), root);

        // all lengths of all strings:
        std::vector<int> allFeedbackLengths(sum(numFeedback));
        mpiLayer.gatherV(localLengths, numFeedback, root, allFeedbackLengths);

        // gather all messages in a single, giant buffer:
        std::vector<int> charsPerNode = mpiLayer.gather((int)localBuffer.size(), root);
        std::vector<char> globalBuffer(sum(charsPerNode));

        mpiLayer.gatherV(localBuffer, charsPerNode, root, globalBuffer);

        // reconstruct strings:
        std::size_t cursor = 0;
        steeringFeedback.resize(0);

        for (std::vector<int>::iterator i = allFeedbackLengths.begin();
             i != allFeedbackLengths.end();
             ++i) {
            std::size_t nextCursor = cursor + *i;
            if(nextCursor >= globalBuffer.size()) {
                break;
            }
            steeringFeedback << std::string(&globalBuffer[cursor],
                                            &globalBuffer[nextCursor]);
            cursor = nextCursor;
        }

        LOG(DBG, "  notifying... steeringFeedback.size(" << MPILayer().rank() << ") == " << steeringFeedback.size() << "\n");
        signal.notify_one();
    }
#endif
};

}

}

#endif
