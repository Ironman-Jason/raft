#include <atomic>
#include <unistd.h>
#include <iostream>
#include <boost/asio.hpp>

#include <raft_timer.h>

#include <gtest/gtest.h>

struct TimerTest : testing::Test {
  boost::asio::io_service ios;
  RaftTimer::RDTimer timer;

  TimerTest(): ios(), timer(ios, 3000, 4000) {
    init_log("test_raft_timer.log");
  }
  ~TimerTest(){}
};

TEST_F(TimerTest, TestWait1){
  std::atomic_bool flag_set = false;
  timer.time([&flag_set](const boost::system::error_code& e){
      std::cout << "e value " << e.message() << std::endl;
      if (e) return;
      flag_set = true;
  });
  std::thread t([this](){ ios.run(); });
  sleep(4);
  EXPECT_TRUE(flag_set);
  t.join();
}

TEST_F(TimerTest, TestInterrupt1){
  std::atomic_bool flag_set = true;
  timer.time([&flag_set](const boost::system::error_code& e){
      std::cout << "e value " << e.message() << std::endl;
      if (e) return;
      flag_set = false;
  });
  std::thread t([this](){ ios.run(); });
  sleep(1);
  timer.reset();
  sleep(4);
  EXPECT_TRUE(flag_set);
  t.join();
}

TEST_F(TimerTest, TestInterrupt2){
  std::atomic_bool flag_set = true;
  timer.time([&flag_set](const boost::system::error_code& e){
      std::cout << "e value " << e.message() << std::endl;
      if (e) return;
      flag_set = false;
  });
  std::thread t([this](){ ios.run(); });
  timer.reset();
  sleep(4);
  EXPECT_TRUE(flag_set);
  t.join();
}


TEST_F(TimerTest, TestInterruptFail){
  std::atomic_bool flag_set = true;
  timer.time([&flag_set](const boost::system::error_code& e){
      std::cout << "e value " << e.message() << std::endl;
      if (e) return;
      flag_set = false;
  });
  std::thread t([this](){ ios.run(); });
  sleep(4);
  timer.reset();
  EXPECT_FALSE(flag_set);
  t.join();
}
