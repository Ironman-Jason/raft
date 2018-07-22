#ifndef RAFT_TIMER
#define RAFT_TIMER

#include <random>

#include <boost/asio.hpp>

#include <raft_util.h>

namespace RaftTimer {
class RDTimer {
  boost::asio::deadline_timer             mTimer;
  std::uniform_int_distribution<unsigned> mDist;
  std::mt19937                            mEng;
public:
  RDTimer(boost::asio::io_service& ios, unsigned min, unsigned max):
    mTimer(ios), mDist(min, max), mEng() {}

  template <typename Callback>
  void time(Callback f){
    mTimer.expires_from_now(boost::posix_time::milliseconds(mDist(mEng)));
    mTimer.async_wait(f);
  }

  void reset(){
    mTimer.cancel();
  }
};
} //RaftTimer

#endif//RAFT_TIMER
