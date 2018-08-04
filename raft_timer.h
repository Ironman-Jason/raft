#ifndef RAFT_TIMER
#define RAFT_TIMER

#include <random>

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <raft_util.h>

namespace Raft {
class RDTimer {
  boost::asio::deadline_timer             mTimer;
  std::uniform_int_distribution<unsigned> mDist;
  std::mt19937                            mEng;
public:
  explicit RDTimer(boost::asio::io_service& ios):
    mTimer(ios), mDist(0, 0), mEng() {}
  RDTimer(boost::asio::io_service& ios, unsigned min_ms, unsigned max_ms):
    mTimer(ios), mDist(min_ms, max_ms), mEng() {}

  template <typename Callback>
  void time(Callback f){
    mTimer.expires_from_now(boost::posix_time::milliseconds(mDist(mEng)));
    mTimer.async_wait(f);
  }

  void reset(){
    mTimer.cancel();
  }

  void change_interval(unsigned min_ms, unsigned max_ms){
    mDist = std::uniform_int_distribution<unsigned>(min_ms, max_ms);
  }

  template <typename Callback>
  void reset(Callback f, unsigned min_ms, unsigned max_ms){
    reset();
    change_interval(min_ms, max_ms);
    time(f);
  }

  template <typename Callback>
  void set(Callback f, unsigned min_ms, unsigned max_ms){
    change_interval(min_ms, max_ms);
    time(f);
  }

  template <typename Callback>
  void reset(Callback f){
    reset();
    time(f);
  }
};
} //Raft

#endif//RAFT_TIMER
