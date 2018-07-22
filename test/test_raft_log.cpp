#include <raft_log.h>

#include <string>
#include <iostream>

#include <gtest/gtest.h>

TEST(RaftLog, TestWrite){
  std::string test_filename("test_log.log");

  std::ofstream cleaner(test_filename.c_str(), std::ofstream::trunc);
  cleaner << "";
  cleaner.close();

  RaftLog::RaftLog<std::string> log(test_filename.c_str());
  log.push_back("block10", 1);
  log.push_back("block21");
  log.push_back("block29");
  log.flush();

  std::ifstream reader(test_filename.c_str());
  std::vector<RaftLog::RaftLogBlock<std::string>> blocks;
  {
    cereal::JSONInputArchive archive(reader);
    archive(blocks);
  }
  reader.close();

  EXPECT_EQ(3, blocks.size());
  EXPECT_STREQ("block10", blocks[0].mData.c_str());
  EXPECT_STREQ("block21", blocks[1].mData.c_str());
  EXPECT_STREQ("block29", blocks[2].mData.c_str());
}

TEST(RaftLog, TestTermIndex){
  std::string test_filename("test_block_term.log");

  std::ofstream cleaner(test_filename.c_str(), std::ofstream::trunc);
  cleaner << "";
  cleaner.close();

  RaftLog::RaftLog<std::string> log(test_filename.c_str());
  log.push_back("block10", 5);
  log.push_back("block11", 5);
  log.push_back("block12", 7);
  log.push_back("block13");
  log.push_back("block14");
  log.push_back("block15");

  EXPECT_EQ(7, log.lastTerm());
  EXPECT_EQ(3, log.lastIndex());
}
