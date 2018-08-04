#ifndef RAFT_SERVER_STATE
#define RAFT_SERVER_STATE

namespace Raft {
enum RaftServerState: unsigned {
  Follower,
  Candidate,
  Master,
};
} //Raft

#endif//RAFT_SERVER_STATE
