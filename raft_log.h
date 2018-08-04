#ifndef RAFT_LOG
#define RAFT_LOG

#include <cassert>
#include <string>
#include <vector>
#include <fstream>

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>

#include <raft_util.h>

namespace Raft {

template <typename T>
struct RaftLogBlock {
  Int mTerm;
  Int mSeqNo;
  T mData;

  RaftLogBlock(Int term, Int no, const T& data): mTerm(term), mSeqNo(no), mData(data) {}
  RaftLogBlock() = default;
  ~RaftLogBlock() = default;

  template <class Archive>
  void serialize(Archive& ar){
    ar(CEREAL_NVP(mTerm), CEREAL_NVP(mSeqNo), CEREAL_NVP(mData));
  }
};

template <typename T>
class RaftLog {
  std::vector<RaftLogBlock<T>> mBlocks;
  std::string mFilename;
  Int mLastLogTerm;
  Int mLastLogIndex;

  void write_out(){
    std::ofstream fs(mFilename.c_str(), std::ofstream::app);
    {
      cereal::JSONOutputArchive archive(fs);
      archive(mBlocks);
    }
    fs.close();
  }

  void read_in(){
    std::ifstream fs(mFilename.c_str());
    if (fs.peek() == std::ifstream::traits_type::eof()) return;

    {
      cereal::JSONInputArchive archive(fs);
      archive(mBlocks);
    }
    fs.close();
  }
public:
  explicit RaftLog(const char* logfile): mFilename(logfile) {
    read_in();

    if (mBlocks.empty()){
      mLastLogTerm = mLastLogIndex = 0;
    } else {
      mLastLogTerm = mBlocks.back().mTerm;
      mLastLogIndex = mBlocks.back().mSeqNo;
    }

    mBlocks.clear();
  }
  ~RaftLog(){ write_out(); }

  void push_back(const T& blk_data, Int new_term = 0){
    Int term = (new_term == 0) ? lastTerm() : new_term;
    Int index = (new_term == 0) ? (lastIndex() + 1) : 0;

    //TODO: take care of this more gracefully
    assert(term > 0);
    assert(term >= lastTerm());
    
    mBlocks.emplace_back(term, index, blk_data);
    mLastLogTerm = term;
    mLastLogIndex = index;
  }

  void flush(){
    write_out();
    mBlocks.clear();
  }

  std::vector<T> all_content(){
    std::vector<T> ret;

    std::ifstream fs(mFilename.c_str());
    if (fs.peek() != std::ifstream::traits_type::eof()){
      std::vector<RaftLogBlock<T>> blocks;
      {
        cereal::JSONInputArchive archive(fs);
        archive(blocks);
      }
      for (auto& block : blocks){
        ret.push_back(block.mData);
      }
    }
    fs.close();

    for (auto& block : mBlocks){
      ret.push_back(block.mData);
    }
    return ret;
  }

  void force_clear(){
    std::ofstream fs(mFilename.c_str(), std::ofstream::trunc);
    fs << "";
    fs.close();
  }

  Int lastTerm() const { return mLastLogTerm; }
  Int lastIndex() const { return mLastLogIndex; }
};

} //Raft

#endif//RAFT_LOG
