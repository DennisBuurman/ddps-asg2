#include <mpi.h>
#include <chrono>
#include <thread>

#include "utility.h"
#include "raft.h"

using Clock = std::chrono::steady_clock;

/* Class Constructor and Destructor*/
Node::Node(int mpi_comm_size, int mpi_rank, int min_timeout, int max_timeout, 
FailType fail_type, float fail_param, Logger &logger) :
    state(follower),
    term(0),
    voted_for(-1),
    mpi_comm_size(mpi_comm_size),
    mpi_rank(mpi_rank),
    min_timeout(min_timeout),
    max_timeout(max_timeout),
    fail_type(fail_type),
    fail_param(fail_param),
    logger(logger),
    buffer(new int) {
        logger.log(info, "Initialized node");
}

Node::~Node() {
    delete buffer;
}

/* Getter/Setter functions */
int Node::get_state() {
    return state;
}

int Node::get_mpi_comm_size() {
    return mpi_comm_size;
}

int Node::get_mpi_rank() {
    return mpi_rank;
}

void Node::simulate_failure() {
    bool failed;
    float chance = fail_param/1000 * POLL_INTERVAL;
    float rand_value = (float) rand()/RAND_MAX;
    Clock::time_point now;
    Clock::duration duration;
    int elapsed_seconds;

    failed = false;
    switch (fail_type) {
        case FailType::chance:
            logger.log(debug, std::to_string(fail_param));
            logger.log(debug, std::to_string(rand_value));
            if (chance > rand_value) {
                failed = true;
            }
            break;
        case FailType::time:
            now = std::chrono::steady_clock::now();
            duration = now - start_time;
            elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
            if (elapsed_seconds >= fail_param) {
                failed = true;
            }
            break;
    }

    if (failed) {
        logger.log(warning, "Node is DEAD");
        std::this_thread::sleep_for(std::chrono::milliseconds(DEAD_TIME));
        logger.log(info, "Node is back online");
    }
}

void Node::broadcast(MsgTag tag) {
    MPI_Request request; //TODO we do not actually use this (fix?)
    for (int i = 0; i < mpi_comm_size; ++i) {
        if (i != mpi_rank) {
            MPI_Isend(
                &term,
                1,
                MPI_INT,
                i,
                tag,
                MPI_COMM_WORLD,
                &request
            );
            //We do not actually care about the outcome
            MPI_Request_free(&request);
        }
    }
}

/* Action subroutine of a follower */
void Node::do_follower() {
    // MPI variables
    MPI_Status status;
    MPI_Request request = MPI_REQUEST_NULL;
    int flag = 0;
    int received_term, received_source, received_tag;

    // Time variables
    auto last_hb = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point now;
    std::chrono::milliseconds duration;
    int timeout = rand(min_timeout, max_timeout);

    while (true) {
        // Try to receive a heartbeat from the leader
        MPI_Irecv(
            buffer, 
            1, 
            MPI_INT, 
            MPI_ANY_SOURCE,
            MPI_ANY_TAG,
            MPI_COMM_WORLD,
            &request
        );
        
        // Receive message until timeout 
        MPI_Test(&request, &flag, &status);
        while (!flag) {
            // Check if node should start new election due to timeout
            now = std::chrono::steady_clock::now();
            duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_hb);
            if (duration.count() > timeout) {
                logger.log(warning, "heartbeat timed out as follower");
                state = candidate;
                MPI_Cancel(&request);
                return;
            }

            // Wait for a bit
            std::this_thread::sleep_for(std::chrono::milliseconds(POLL_INTERVAL));

            // Check again if message is received
            MPI_Test(&request, &flag, &status);
        }

        // Process received message
        received_term = *buffer;
        received_source = status.MPI_SOURCE;
        received_tag = status.MPI_TAG; 

        if (received_term > term) {
            // To this node, sender is newly elected leader or candidate
            logger.log(info, "received message with higher term as follower");
            term = received_term;
            voted_for = -1;
        }

        switch (received_tag) {
            case heartbeat:
                logger.log(debug, "received heartbeat as follower");
                if (received_term >= term) {
                    // Sender is a valid leader
                    logger.log(debug, "received valid heartbeat as follower");
                    last_hb = std::chrono::steady_clock::now();
                }
                break;
            case vote_request:
                logger.log(debug, "received vote request as follower");
                if (received_term < term) {
                    logger.log(debug, "granted vote request as follower");
                } else if (voted_for == -1 || voted_for == received_tag) {
                    MPI_Isend(
                        &term, // TODO: if jank, alloc unit
                        1,
                        MPI_INT,
                        received_source,
                        vote_response,
                        MPI_COMM_WORLD,
                        &request
                    );
                    voted_for = received_source;
                }
                break;
            default:
                //This message was probably not meant for us (anymore)
                logger.log(warning, "received unexpected message as follower");
                break;
        }
    }
}

/* Action subroutine of a candidate */
void Node::do_candidate() {
    bool *votes = new bool[mpi_comm_size](); // allocate and init voting buffer
    MPI_Request request;
    MPI_Status status;
    int received_term, received_source, received_tag;
    int flag = -1;
    
    // Time variables
    int election_time = rand(min_timeout, max_timeout);
    int broadcast_time = BROADCAST_INTERVAL+1;
    auto election_start = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point now;
    std::chrono::steady_clock::time_point prev_broadcast;
    std::chrono::milliseconds duration;

    // Update election term and vote
    term++;
    voted_for = mpi_rank; // vote for yourself
    votes[mpi_rank] = 1;

    // Start voting process
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - now);
    while (duration.count() < election_time) {
        // Broadcast if interval expired
        if (broadcast_time > BROADCAST_INTERVAL) {
            broadcast(vote_request);
            broadcast_time = 0;
        } else {
            broadcast_time += std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - prev_broadcast).count();
            prev_broadcast = std::chrono::steady_clock::now();
        }

        //Receive any messages
        if (flag != 0) {
            MPI_Irecv(
                buffer,
                1,
                MPI_INT,
                MPI_ANY_SOURCE,
                MPI_ANY_TAG,
                MPI_COMM_WORLD,
                &request
            );
            flag = 0;
        }

        // Check if message received
        MPI_Test(&request, &flag, &status);

        //Process message if received
        if (flag == 1) {
            received_term = *buffer;
            received_source = status.MPI_SOURCE;
            received_tag = status.MPI_TAG;
            if (received_term > term) {
                logger.log(info, "received message with higher term as candidate");
                term = received_term;
                voted_for = -1;
                state = follower;
                return;
            }
            switch(received_tag) {
                case heartbeat:
                    //Check if there is another leader already
                    logger.log(debug, "received heartbeat from a leader as candidate");
                    if (term <= received_term) {
                        logger.log(warning, "received heartbeat from newer leader as candidate");
                        term = received_term;
                        voted_for = -1;
                        state = follower;
                        return;
                    }
                    break;
                case vote_response:
                    //Process received vote
                    logger.log(debug, "received vote as candidate");
                    if (received_term == term) {
                        votes[received_source] = true;
                        if (majority(votes, mpi_comm_size)) {
                            state = leader;
                            return;
                        }
                    }
                    break;
                case vote_request:
                    //Deny (ignore)
                    logger.log(debug, "ignoring vote request as we are candidate");
                    break;
                default:
                    //This message was probably not meant for us (anymore)
                    logger.log(info, "received unhandled message as candidate");
                    break;
            }
        }
        // Wait for a bit
        std::this_thread::sleep_for(std::chrono::milliseconds(POLL_INTERVAL));

        now = std::chrono::steady_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - election_start);
    }
}

/* Action subroutine of a leader */
void Node::do_leader() {
    int flag = -1;
    int received_term, received_source, received_tag;
    MPI_Request request;
    MPI_Status status;

    // Time variables
    int broadcast_time = BROADCAST_INTERVAL+1;
    std::chrono::steady_clock::time_point prev_broadcast;

    // Periodically send hardbeats and check for incoming messages
    while (true) {
        // Broadcast heartbeat if interval expired
        if (broadcast_time > BROADCAST_INTERVAL) {
            logger.log(debug, "broadcasting heartbeat");
            broadcast(heartbeat);
            broadcast_time = 0;
        } else {
            broadcast_time += std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - prev_broadcast).count();
            prev_broadcast = std::chrono::steady_clock::now();
            //printf("current broadcast time: %d\n", broadcast_time);
        }

        // Try to receive a message
        if (flag != 0) {
            MPI_Irecv(
                buffer,
                1,
                MPI_INT,
                MPI_ANY_SOURCE,
                MPI_ANY_TAG,
                MPI_COMM_WORLD,
                &request
            );
            flag = 0;
        }

        // Check if message received
        MPI_Test(&request, &flag, &status);

        // Process message if received
        if (flag == 1) {
            received_term = *buffer;
            received_source = status.MPI_SOURCE;
            received_tag = status.MPI_TAG;
            if (received_term > term) {
                logger.log(info, "received message with higher term");
                voted_for = -1;
                if (received_tag != vote_request) {
                    term = received_term;
                    state = follower;
                    return;
                } //Else handle in the switch 
            }
            switch (received_tag) {
                case heartbeat:
                    logger.log(warning, "received heartbeat from node with higher term");
                    break;
                case vote_request:
                    if (received_term >= term && (voted_for == -1 || voted_for == received_source)) {
                        MPI_Isend(
                            &received_term,
                            1,
                            MPI_INT,
                            received_source,
                            vote_response,
                            MPI_COMM_WORLD,
                            &request
                        );
                        term = received_term;
                        voted_for = received_source;
                        state = follower;
                        return;
                    }
                    break;
                case vote_response:
                    logger.log(info, "ignored vote response");
                    break;
                default:
                    logger.log(warning, "unhandled message");
                    break;
            }
        }

        // Wait for a bit
        simulate_failure();
        std::this_thread::sleep_for(std::chrono::milliseconds(POLL_INTERVAL));
    }
}

/* Perform leader election algorithm */
void Node::run() {
    while (true) {
        // Perform action corresponding to role
        switch (state) {
            case follower:
                logger.log(info, "state changed to follower");
                do_follower();
                break;
            case candidate:
                logger.log(info, "state changed to candidate");
                do_candidate();
                break;
            case leader:
                logger.log(info, "state changed to leader");
                do_leader();
                break;
            case dead:
                logger.log(warning, "state changed to DEAD");
                std::this_thread::sleep_for(std::chrono::milliseconds(DEAD_TIME));

            default:
                logger.log(warning, "unsopported node state found");
                break;
        }
    }
}