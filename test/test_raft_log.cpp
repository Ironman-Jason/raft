#include <raft_log.h>

#include <string>
#include <vector>
#include <iostream>

#include <gtest/gtest.h>

struct TestRaftLog : ::testing::Test {
  std::string test_filename;
  std::vector<Raft::RaftLogBlock<std::string>> test_blocks;

  TestRaftLog(): test_filename("test_log.log") {
    std::ofstream cleaner(test_filename.c_str(), std::ofstream::trunc);
    cleaner.close();
  }
  ~TestRaftLog(){}
};

TEST_F(TestRaftLog, TestWrite1){
  Raft::RaftLog<std::string> log(test_filename.c_str());
  std::vector<std::string> expected;
  expected.push_back("block10");
  expected.push_back("block11");
  expected.push_back("block12");
  bool success = log.push_back_at(0, 0, 1, 0, expected);
  ASSERT_TRUE(success);
  log.flush();

  std::ifstream reader(test_filename.c_str());
  {
    cereal::JSONInputArchive ar(reader);
    ar(test_blocks);
  }
  reader.close();

  ASSERT_EQ(3, test_blocks.size());
  EXPECT_STREQ("block10", test_blocks[0].mData.c_str());
  EXPECT_STREQ("block11", test_blocks[1].mData.c_str());
  EXPECT_STREQ("block12", test_blocks[2].mData.c_str());
}

TEST_F(TestRaftLog, TestWrite2){
  Raft::RaftLog<std::string> log(test_filename.c_str());
  std::vector<std::string> part1, part2;
  part1.push_back("block10");
  part1.push_back("block11");
  part1.push_back("block12");
  part2.push_back("block20");
  part2.push_back("block21");
  part2.push_back("block22");
  bool success1 = log.push_back_at(0, 0, 1, 0, part1);
  bool success2 = log.push_back_at(1, 1, 2, 0, part2);
  ASSERT_TRUE(success1);
  ASSERT_TRUE(success2);
  log.flush();

  std::ifstream reader(test_filename.c_str());
  {
    cereal::JSONInputArchive ar(reader);
    ar(test_blocks);
  }
  reader.close();

  ASSERT_EQ(5, test_blocks.size());
  EXPECT_STREQ("block10", test_blocks[0].mData.c_str());
  EXPECT_STREQ("block11", test_blocks[1].mData.c_str());
  EXPECT_STREQ("block20", test_blocks[2].mData.c_str());
  EXPECT_STREQ("block21", test_blocks[3].mData.c_str());
  EXPECT_STREQ("block22", test_blocks[4].mData.c_str());
  EXPECT_EQ(1, test_blocks[0].mTerm);
  EXPECT_EQ(2, test_blocks[2].mTerm);
  EXPECT_EQ(1, test_blocks[1].mSeqNo);
  EXPECT_EQ(0, test_blocks[2].mSeqNo);
  EXPECT_EQ(1, test_blocks[3].mSeqNo);
  EXPECT_EQ(2, test_blocks[4].mSeqNo);
}

TEST_F(TestRaftLog, TestTermIndex){
  Raft::RaftLog<std::string> log(test_filename.c_str());
  std::vector<std::string> expected;
  expected.push_back("block10");
  expected.push_back("block11");
  expected.push_back("block12");
  expected.push_back("block13");
  expected.push_back("block14");
  expected.push_back("block15");
  bool success = log.push_back_at(0, 0, 5, 2, expected);
  ASSERT_TRUE(success);

  EXPECT_EQ(5, log.lastTerm());
  EXPECT_EQ(7, log.lastIndex());
}
