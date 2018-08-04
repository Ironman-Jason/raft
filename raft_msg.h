#ifndef RAFT_MSG
#define RAFT_MSG

#include <string>
#include <sstream>

#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>

#include <raft_util.h>

namespace Raft {

enum RaftMsgType: unsigned {
  RVoteReq,
  AEntReq,
  InSnapReq,
  Rep,
  Unknown,
};

struct Type {
  RaftMsgType type;

  explicit Type(RaftMsgType t): type(t) {}
  Type() = default;

  template <typename Archive>
  void serialize(Archive& archive){
    archive(type);
  }
};

struct RequestVote {
  Int term;
  std::string candidateId;
  Int lastLogTerm;
  Int lastLogIndex;

  RequestVote(Int t, const std::string& cid, Int llt, Int lli):
    term(t), candidateId(cid), lastLogTerm(llt), lastLogIndex(lli) {}
  RequestVote() = default;
  ~RequestVote() = default;

  static const char* type(){ return "RequestVote"; }
  static RaftMsgType etype(){ return RVoteReq; }

  template <class Archive>
  void serialize(Archive& archive){
    archive(term, candidateId, lastLogTerm, lastLogIndex);
  }
};

template <typename T>
struct AppendEntries {
  Int term;
  std::string leaderId;
  Int prevLogTerm;
  Int prevLogIndex;
  std::vector<T> entries;
  Int leaderCommit;

  AppendEntries(Int t, const std::string& lid, Int plt, Int pli, Int lc):
    term(t), leaderId(lid), prevLogTerm(plt), prevLogIndex(pli), leaderCommit(lc) {}
  AppendEntries() = default;
  ~AppendEntries() = default;

  static const char* type(){ return "AppendEntries"; }
  static RaftMsgType etype(){ return AEntReq; }

  void push_back(const T& e){
    entries.push_back(e);
  }

  template <typename Archive>
  void serialize(Archive& archive){
    archive(term, leaderId, prevLogTerm, prevLogIndex, entries, leaderCommit);
  }
};

struct InstallSnapshot {
  Int term;
  std::string leaderId;
  Int lastIncludedTerm;
  Int lastIncludedIndex;
  Int offset;
  std::vector<std::byte> data;
  bool done;

  InstallSnapshot(Int t, const std::string& lid, Int lit, Int lii, Int o, const std::vector<std::byte>& d, bool done):
    term(t), leaderId(lid), lastIncludedTerm(lit), lastIncludedIndex(lii), offset(o), data(d), done(done) {}
  InstallSnapshot() = default;
  ~InstallSnapshot() = default;

  static const char* type(){ return "InstallSnapshot"; }
  static RaftMsgType etype(){ return InSnapReq; }

  template <typename Archive>
  void serialize(Archive& archive){
    archive(term, leaderId, lastIncludedTerm, lastIncludedIndex, offset, data, done);
  }
};

struct Reply {
  Int term;
  bool voteGranted;

  Reply(Int t, bool vg):
    term(t), voteGranted(vg) {}
  Reply() = default;
  ~Reply() = default;

  static const char* type(){ return "Reply"; }
  static RaftMsgType etype(){ return Rep; }

  template <typename Archive>
  void serialize(Archive& archive){
    archive(term, voteGranted);
  }
};

} //Raft

#endif //RAFT_MSG
