#ifndef RAFT_ELECTION_SERVER
#define RAFT_ELECTION_SERVER

#include <raft_util.h>
#include <raft_config.h>
#include <raft_msg.h>
#include <raft_rpc.h>
#include <raft_timer.h>
#include <raft_server_state.h>

#include <cassert>
#include <vector>
#include <string>
#include <atomic>
#include <mutex>

#include <boost/asio.hpp>

namespace Raft {

using NodeName = std::string;
using AddressBook = std::map<NodeName, Address>;

//macro shortcuts
#define SET_RAFT_STATE(lock, var, new_state)\
  {\
    std::lock_guard<std::mutex> guard(lock);\
    var = new_state;\
  }

#define RESET_RAFT_STATE(lock, var)\
  {\
    std::lock_guard<std::mutex> guard(lock);\
    var.clear();\
  }

#define GET_RAFT_STATE(lock, var, state_var)\
  {\
    std::lock_guard<std::mutex> guard(lock);\
    var = state_var;\
  }

// A Raft node that only do master election
class ElectionServer {
  RPC<char>            mRPC;
  RDTimer              mTimer;
  //ready only properties
  NodeName             mName;
  AddressBook          mAddressBook;
  std::vector<Address> mNodes;
  unsigned             mElectionTimeoutMin;
  unsigned             mElectionTimeoutMax;
  unsigned             mHeartbeatInterval;
  //state variables
  std::mutex           mVotedForLock;   //lock for access mVotedFor
  std::string          mVotedFor;
  std::mutex           mStateLock;      //lock for access mState
  RaftServerState      mState;
  std::mutex           mMasterNameLock; //lock for master name
  std::string          mMasterName;
  std::atomic<Int>     mCurrentTerm;
  std::atomic<Int>     mVoteCount;

  void trigger_election(const boost::system::error_code& ecode){
    namespace sev = boost::log::trivial;

    if (ecode == boost::system::errc::operation_canceled) return;
    if (ecode){
      BOOST_LOG_SEV(logger(), sev::error) << __FUNCTION__ << " unexpected error: " << ecode.message();
      BOOST_LOG_SEV(logger(), sev::error) << __FUNCTION__ << " shutting down server";
      assert(false);
      return;
    }

    SET_RAFT_STATE(mStateLock, mState, Candidate);
    SET_RAFT_STATE(mVotedForLock, mVotedFor, mName);
    Int lastTerm = mCurrentTerm.load();
    mCurrentTerm++;
    mVoteCount = 1;

    BOOST_LOG_SEV(logger(), sev::info) << __FUNCTION__ << " " << mName << " trigger election. term " << (lastTerm + 1);

    mRPC.rpc_async_broadcast<Reply>(mNodes, RequestVote(mCurrentTerm.load(), mName, lastTerm, 0), [this](const Reply& rep){ vote_count(rep); });
    mTimer.set(std::bind(&ElectionServer::vote_timeout, this, std::placeholders::_1), mElectionTimeoutMin, mElectionTimeoutMax);
  }

  void vote_timeout(const boost::system::error_code& ecode){
    namespace sev = boost::log::trivial;

    if (ecode == boost::system::errc::operation_canceled) return;
    if (ecode){
      BOOST_LOG_SEV(logger(), sev::error) << __FUNCTION__ << " unexpected error: " << ecode.message();
      BOOST_LOG_SEV(logger(), sev::error) << __FUNCTION__ << " shutting down server";
      assert(false);
    }

    BOOST_LOG_SEV(logger(), sev::info) << __FUNCTION__ << " " << mName << " vote timeout, re-triggering election";

    // we assume vote count has not reached critical, and we did not receive
    // alternative master for the current or future term
    trigger_election(boost::system::error_code());
  }

  void vote_count(const Reply& reply){
    namespace sev = boost::log::trivial;

    if (reply.term == mCurrentTerm.load() && reply.voteGranted)
      mVoteCount++;

    BOOST_LOG_SEV(logger(), sev::info) << __FUNCTION__ << " received vote, granted: " << reply.voteGranted;

    if (mVoteCount.load() > (mNodes.size() + 1) / 2){
      {
        std::lock_guard<std::mutex> guard(mStateLock);
        if (mState == Master) return;
        mState = Master;
      }
      promote_master_name(mName);

      BOOST_LOG_SEV(logger(), sev::info) << __FUNCTION__ << " " << mName << " become master";

      mRPC.rpc_async_broadcast<Reply>(mNodes, AppendEntries<char>(mCurrentTerm.load(), mName, mCurrentTerm.load(), 0, 0), [](const Reply&){});
      mTimer.reset(std::bind(&ElectionServer::heartbeat, this, std::placeholders::_1), mHeartbeatInterval, mHeartbeatInterval);
    }
  }

  void heartbeat(const boost::system::error_code& ecode){
    namespace sev = boost::log::trivial;

    if (ecode == boost::system::errc::operation_canceled) return;
    if (ecode){
      BOOST_LOG_SEV(logger(), sev::error) << __FUNCTION__ << " unexpected error: " << ecode.message();
      BOOST_LOG_SEV(logger(), sev::error) << __FUNCTION__ << " shutting down server";
      assert(false);
    }

    BOOST_LOG_SEV(logger(), sev::info) << __FUNCTION__ << " " << mName << " sending heartbeat";

    mRPC.rpc_async_broadcast<Reply>(mNodes, AppendEntries<char>(mCurrentTerm.load(), mName, mCurrentTerm.load(), 0, 0), [](const Reply&){});
    mTimer.time(std::bind(&ElectionServer::heartbeat, this, std::placeholders::_1));
  }

  Reply request_vote(const RequestVote& obj){
    namespace sev = boost::log::trivial;

    Reply ret(obj.term, false);
    RaftServerState local_state;
    GET_RAFT_STATE(mStateLock, local_state, mState);

    BOOST_LOG_SEV(logger(), sev::info) << __FUNCTION__ << " " << mName << " received RequestVote at state " << local_state;

    switch (local_state){
    case Follower:
      if (obj.term > mCurrentTerm.load()){
        std::lock_guard<std::mutex> guard(mVotedForLock);
        if (mVotedFor.length() == 0){

          BOOST_LOG_SEV(logger(), sev::info) << __FUNCTION__ << " vote granted";

          mVotedFor = obj.candidateId;
          ret.voteGranted = true;
        }
      }
      break;
    case Candidate:
    case Master:
      if (obj.term > mCurrentTerm.load()){
        BOOST_LOG_SEV(logger(), sev::info) << __FUNCTION__ << " vote granted";

        SET_RAFT_STATE(mVotedForLock, mVotedFor, obj.candidateId);
        SET_RAFT_STATE(mStateLock, mState, Follower);
        mTimer.reset(std::bind(&ElectionServer::trigger_election, this, std::placeholders::_1), mElectionTimeoutMin, mElectionTimeoutMax);
        ret.voteGranted = true;
      }
      break;
    default: assert(false); break;
    }
    return ret;
  }

  Reply append_entries(const AppendEntries<char>& obj){
    namespace sev = boost::log::trivial;

    Reply ret(obj.term, false);
    RaftServerState local_state;
    GET_RAFT_STATE(mStateLock, local_state, mState);

    BOOST_LOG_SEV(logger(), sev::info) << __FUNCTION__ << " received AppendEntries at state: " << local_state;

    switch (local_state){
    case Follower:
      if (obj.term > mCurrentTerm.load()){
        mCurrentTerm = obj.term;
        promote_master_name(obj.leaderId);
      }
      mTimer.reset(std::bind(&ElectionServer::trigger_election, this, std::placeholders::_1));
      ret.voteGranted = true;
      break;
    case Candidate:
      if (obj.term >= mCurrentTerm.load()){
        BOOST_LOG_SEV(logger(), sev::info) << __FUNCTION__ << " become follower";

        demote_to_follower(obj.term, obj.leaderId);
        ret.voteGranted = true;
      }
      break;
    case Master:
      if (obj.term > mCurrentTerm.load()){
        BOOST_LOG_SEV(logger(), sev::info) << __FUNCTION__ << " become follower";

        demote_to_follower(obj.term, obj.leaderId);
        ret.voteGranted = true;
      }
      break;
    default: assert(false); break;
    }
    return ret;
  }

  Reply install_snapshot(const InstallSnapshot& obj){
    //do nothing: we should not need this RPC for election only
    return Reply(obj.term, false);
  }

  void demote_to_follower(Int new_term, const std::string& master_name){
    mCurrentTerm = new_term;
    SET_RAFT_STATE(mStateLock, mState, Follower);
    promote_master_name(master_name);
    mTimer.reset(std::bind(&ElectionServer::trigger_election, this, std::placeholders::_1), mElectionTimeoutMin, mElectionTimeoutMax);
  }

  void promote_master_name(const std::string& master_name){
    RESET_RAFT_STATE(mVotedForLock, mVotedFor);
    SET_RAFT_STATE(mMasterNameLock, mMasterName, master_name);
  }

public:
  ElectionServer(boost::asio::io_service& ios, const NodeName& name, const ServerConfig& config, const RaftProperty& property):
    mRPC(ios, config.address.port()),
    mTimer(ios, property.election_timeout_min, property.election_timeout_max),
    mName(name),
    mElectionTimeoutMin(property.election_timeout_min),
    mElectionTimeoutMax(property.election_timeout_max),
    mHeartbeatInterval(property.heartbeat_interval),
    mState(Follower),
    mCurrentTerm(0),
    mVoteCount(0) {
    namespace sev = boost::log::trivial;

    for (RaftServerMap::const_iterator it = property.server_map.cbegin(); it != property.server_map.cend(); ++it){
      mAddressBook.insert(std::make_pair((*it).first, (*it).second.address));
      if ((*it).second.address != config.address)
        mNodes.push_back((*it).second.address);
    }

    BOOST_LOG_SEV(logger(), sev::info) << "ElectionServer " << mName << " started. known nodes:";
    for (AddressBook::iterator it = mAddressBook.begin(); it != mAddressBook.end(); ++it)
      BOOST_LOG_SEV(logger(), sev::info) << (*it).first << " = " << (*it).second.ip() << ":" << (*it).second.port();

    mRPC.rpc_persistent_async_dispatch(
      [this](const RequestVote& obj){
        return request_vote(obj);
      },
      [this](const AppendEntries<char>& obj){
        return append_entries(obj);
      },
      [this](const InstallSnapshot& obj){
        return install_snapshot(obj);
      }
    );
    mTimer.time(std::bind(&ElectionServer::trigger_election, this, std::placeholders::_1));
  }

  std::string get_master_ip(){
    namespace sev = boost::log::trivial;
    std::string master_name;
    GET_RAFT_STATE(mMasterNameLock, master_name, mMasterName);
    if (master_name.length() > 0)
      return mAddressBook[master_name].ip();
    else
      return std::string();
  }

  bool is_master(){
    RaftServerState local_state;
    GET_RAFT_STATE(mStateLock, local_state, mState);
    return local_state == Master;
  }
}; //ElectionServer

#undef SET_RAFT_STATE
#undef RESET_RAFT_STATE
#undef GET_RAFT_STATE

} //Raft

#endif//RAFT_ELECTION_SERVER
