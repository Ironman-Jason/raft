#ifndef RAFT_LOG
#define RAFT_LOG

#include <cassert>
#include <type_traits>
#include <string>
#include <vector>
#include <fstream>
#include <optional>
#include <algorithm>

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
  RaftLogBlock(Int term, Int no): mTerm(term), mSeqNo(no), mData() {}
  RaftLogBlock() = default;
  ~RaftLogBlock() = default;

  template <class Archive>
  void serialize(Archive& ar){
    ar(CEREAL_NVP(mTerm), CEREAL_NVP(mSeqNo), CEREAL_NVP(mData));
  }
};

template <typename T>
bool operator<(const RaftLogBlock<T>& a, const RaftLogBlock<T>& b){
  if (a.mTerm < b.mTerm)                         return true;
  if (a.mTerm == b.mTerm && a.mSeqNo < b.mSeqNo) return true;
  else                                           return false;
}

template <typename T>
class RaftLog {
  std::vector<RaftLogBlock<T>> mBlocks;
  std::string mFilename;
  Int mLastLogTerm;
  Int mLastLogIndex;

  void write_out(){
    std::ofstream fs(mFilename.c_str(), std::ofstream::trunc);
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

  void clear_logfile(){
    std::ofstream cleaner;
    cleaner.open(mFilename.c_str(), std::ofstream::out | std::ofstream::trunc);
    cleaner.close();
  }

  void append_blocks(Int term, Int idx, const std::vector<T>& blocks){
    Int idx_accu = idx;
    for (typename std::decay<decltype(blocks)>::type::const_iterator it = std::cbegin(blocks); it != std::cend(blocks); ++it)
      mBlocks.emplace_back(term, idx_accu++, (*it));
    mLastLogTerm = term;
    mLastLogIndex = idx_accu - 1;
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

    clear_logfile();
  }
  ~RaftLog(){ write_out(); }

  //NOTE: term 0 is reserved to represent no term and no data; there should be
  //no block inserted with term 0
  bool push_back_at(Int term, Int idx, Int new_term, Int new_idx, const std::vector<T>& blocks){
    //if there is no log yet
    if (term == 0){
      append_blocks(new_term, new_idx, blocks);
      return true;
    }

    typename std::decay<decltype(mBlocks)>::type::iterator it = std::lower_bound(std::begin(mBlocks), std::end(mBlocks), RaftLogBlock<T>(term, idx));

    if (it == std::end(mBlocks)) return false;

    mBlocks.erase(++it, std::end(mBlocks));

    append_blocks(new_term, new_idx, blocks);
    return true;
  }

  std::optional<T> get(Int term, Int idx) const {
    if (term == 0) return std::nullopt;
    if (mLastLogTerm < term) return std::nullopt;

    typename std::decay<decltype(mBlocks)>::type::iterator it = std::lower_bound(std::begin(mBlocks), std::end(mBlocks), RaftLogBlock<T>(term, idx));

    if (it == std::end(mBlocks))
      return std::nullopt;

    return (*it).mData;
  }

  //erase until block where term = term and seqno = idx
  void erase_until(Int term, Int idx){
    mBlocks.erase(std::remove_if(std::begin(mBlocks), std::end(mBlocks), [term, idx](RaftLogBlock<T>& bk){
      if (bk.mTerm < term)                     return true;
      if (bk.mTerm == term && bk.mSeqNo < idx) return true;
      else                                     return false;
    }));
  }

  void flush(){
    write_out();
  }

  void force_clear(){
    clear_logfile();
  }

  Int lastTerm() const { return mLastLogTerm; }
  Int lastIndex() const { return mLastLogIndex; }
};

} //Raft

#endif//RAFT_LOG
