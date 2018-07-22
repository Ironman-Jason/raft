#include <raft_msg.h>

#include <string>
#include <sstream>
#include <iostream>

#include <gtest/gtest.h>

TEST(Type, UBinaryTest){
  std::stringstream ss;
  {
    cereal::PortableBinaryOutputArchive oarchive(ss);
    RaftMsg::Type r(RaftMsg::RVoteReq);
    oarchive(r);
  }

  std::cout << "Type binary: " << ss.str() << std::endl;

  RaftMsg::Type t;
  {
    cereal::PortableBinaryInputArchive iarchive(ss);
    iarchive(t);
  }

  EXPECT_EQ(RaftMsg::RVoteReq, t.type);
}

TEST(RequestVote, UBinaryTest){
  std::stringstream ss;
  {
    cereal::PortableBinaryOutputArchive oarchive(ss);
    RaftMsg::RequestVote r(32799, "vortex", 88, 72);
    oarchive(r);
  }

  std::cout << "RequestVote binary: " << ss.str() << std::endl;

  RaftMsg::RequestVote t;
  {
    cereal::PortableBinaryInputArchive iarchive(ss);
    iarchive(t);
  }

  EXPECT_EQ(32799, t.term);
  EXPECT_STREQ("vortex", t.candidateId.c_str());
  EXPECT_EQ(88, t.lastLogTerm);
  EXPECT_EQ(72, t.lastLogIndex);
}

TEST(AppendEntries, UBinaryTest){
  std::stringstream ss;
  {
    cereal::PortableBinaryOutputArchive oarchive(ss);
    RaftMsg::AppendEntries<std::string> r(7531, "work please", 91, 99999999, 1000);
    r.push_back("entry 1");
    r.push_back("entry 2");
    r.push_back("entry 3");
    r.push_back("entry 4");
    oarchive(r);
  }

  std::cout << "AppendEntries binary: " << ss.str() << std::endl;

  RaftMsg::AppendEntries<std::string> t;
  {
    cereal::PortableBinaryInputArchive iarchive(ss);
    iarchive(t);
  }

  EXPECT_EQ(7531, t.term);
  EXPECT_STREQ("work please", t.leaderId.c_str());
  EXPECT_EQ(91, t.prevLogTerm);
  EXPECT_EQ(99999999, t.prevLogIndex);
  EXPECT_EQ(4, t.entries.size());
  EXPECT_STREQ("entry 1", t.entries[0].c_str());
  EXPECT_STREQ("entry 2", t.entries[1].c_str());
  EXPECT_STREQ("entry 3", t.entries[2].c_str());
  EXPECT_STREQ("entry 4", t.entries[3].c_str());
  EXPECT_EQ(1000, t.leaderCommit);
}

TEST(AppendEntries, LargeTest){
  std::stringstream ss;
  {
    cereal::PortableBinaryOutputArchive oarchive(ss);
    RaftMsg::AppendEntries<std::string> r(7531, "work please", 91, 99999999, 1000);
    for (size_t i = 0; i < 100000; ++i){
      std::string msg("hello ");
      msg += std::to_string(i);
      r.push_back(msg);
    }
    oarchive(r);
  }

  std::cout << "Size: " << ss.str().length() << std::endl;

  RaftMsg::AppendEntries<std::string> t;
  {
    cereal::PortableBinaryInputArchive iarchive(ss);
    iarchive(t);
  }

  EXPECT_EQ(7531, t.term);
  EXPECT_STREQ("work please", t.leaderId.c_str());
  EXPECT_EQ(91, t.prevLogTerm);
  EXPECT_EQ(99999999, t.prevLogIndex);
  EXPECT_EQ(100000, t.entries.size());
  EXPECT_EQ(1000, t.leaderCommit);
}

TEST(InstallSnapshot, UBinaryTest){
  //TODO
}

TEST(Reply, UBinaryTest){
  std::stringstream ss;
  {
    cereal::PortableBinaryOutputArchive oarchive(ss);
    RaftMsg::Reply r(8080, true);
    oarchive(r);
  }

  std::cout << "Reply binary: " << ss.str() << std::endl;
  std::cout << "Size: " << ss.str().length() << std::endl;

  RaftMsg::Reply t;
  {
    cereal::PortableBinaryInputArchive iarchive(ss);
    iarchive(t);
  }

  EXPECT_EQ(8080, t.term);
  EXPECT_EQ(true, t.voteGranted);
}

