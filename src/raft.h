#ifndef RAFT_H
#define RAFT_H

#include <chrono>

#include "logger.h"

/* Global variables and constructs */
enum NodeState {leader, follower, candidate, dead}; // available states for a node
enum MsgTag {heartbeat, vote_request, vote_response}; // different message types
enum class FailType {chance, time};

static const int POLL_INTERVAL=10; // time between heartbeat polls
static const int BROADCAST_INTERVAL=1000;
static const int DEAD_TIME=60000; // time to be dead (1 minute)

class Node {
    public:
        /* Constructor and Destructor */
        Node(
            int mpi_comm_size, 
            int mpi_rank, 
            int min_timeout,
            int max_timeout,
            FailType 
            fail_type, 
            float fail_param, 
            Logger &logger);
        ~Node();
        
        /* Member functions */
        void do_follower(); // TODO: implement
        void do_candidate(); // TODO: implement
        void do_leader(); // TODO: implement
        void run(); // TODO: implement

        /* Getter/Setter functions */
        int get_term();
        int get_state();
        int get_mpi_comm_size();
        int get_mpi_rank();
        void simulate_failure();
        void broadcast(MsgTag tag);

    private:
        /* Class variables */
        NodeState state;
        int term;
        int voted_for;
        int mpi_comm_size;
        int mpi_rank;
        int min_timeout;
        int max_timeout;
        FailType fail_type;
        float fail_param;
        std::chrono::steady_clock::time_point start_time;
        Logger logger;

        //TODO still not a fan of this
        int *buffer;
};

#endif