#include <atomic>
#include <unistd.h>
#include <ctime>
#include <iostream>
#include <boost/asio.hpp>

#include <raft_timer.h>

#include <gtest/gtest.h>

struct TimerTest : testing::Test {
  boost::asio::io_service ios;
  Raft::RDTimer timer;

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

struct BoundaryCaseTest : testing::Test {
  boost::asio::io_service ios;
  Raft::RDTimer timer;

  BoundaryCaseTest(): ios(), timer(ios, 10000, 10001) {
    init_log("test_raft_timer.log");
  }
  ~BoundaryCaseTest(){}
};

TEST_F(BoundaryCaseTest, testMultipleReset){
  //this test case should not generate any error
  time_t call_time = 0;
  timer.time([&call_time](const boost::system::error_code& ecode){
    EXPECT_EQ(boost::system::errc::operation_canceled, ecode);
    call_time = time(NULL);
  });
  std::cout << "start time: " << time(NULL) << std::endl;
  std::thread t1([this](){ ios.run(); });
  std::thread t2([this](){ timer.reset(); });
  std::thread t3([this](){ timer.reset(); });
  std::thread t4([this](){ timer.reset(); });

  t1.join();
  std::cout << "call time " << call_time << std::endl;
  t2.join();
  t3.join();
  t4.join();
}

TEST_F(BoundaryCaseTest, testResetTimeImmediate){
  time_t call_time = 0;
  timer.time([&call_time](const boost::system::error_code& ecode){
    EXPECT_EQ(boost::system::errc::operation_canceled, ecode);
    call_time = time(NULL);
  });
  time_t start_time = time(NULL);
  std::thread t([this](){ ios.run(); });
  usleep(500000);
  timer.reset();
  EXPECT_TRUE(call_time - start_time < 10);
  std::cout << "start time: " << start_time << " call time: " << call_time << std::endl;

  t.join();
}

TEST_F(BoundaryCaseTest, testResetThenRestart1){
  time_t call_time = 0;
  timer.time([](const boost::system::error_code& ecode){
    std::cout << "handler1 called" << std::endl;
    EXPECT_EQ(boost::system::errc::operation_canceled, ecode);
  });
  time_t start_time = time(NULL);
  std::thread t1([this](){ ios.run(); });
  timer.reset();
  timer.time([&call_time](const boost::system::error_code& ecode){
    std::cout << "handler2 called" << std::endl;
    EXPECT_EQ(boost::system::errc::success, ecode);
    call_time = time(NULL);
  });
  t1.join();
  std::thread t2([this](){ ios.run(); });
  t2.join();
  std::cout << "time delay: " << (call_time - start_time) << std::endl;
  EXPECT_TRUE(call_time - start_time >= 10);
}

TEST_F(BoundaryCaseTest, testResetThenRestart2){
  time_t call_time = 0;
  timer.time([](const boost::system::error_code& ecode){
    std::cout << "handler1 called" << std::endl;
    EXPECT_EQ(boost::system::errc::operation_canceled, ecode);
  });
  time_t start_time = time(NULL);
  std::thread t1([this](){ ios.run(); });
  usleep(800000);
  timer.reset();
  timer.time([&call_time](const boost::system::error_code& ecode){
    std::cout << "handler2 called" << std::endl;
    EXPECT_EQ(boost::system::errc::success, ecode);
    call_time = time(NULL);
  });
  t1.join();
  std::thread t2([this](){ ios.run(); });
  t2.join();
  std::cout << "time delay: " << (call_time - start_time) << std::endl;
  EXPECT_TRUE(call_time - start_time > 10);
}

TEST_F(BoundaryCaseTest, testResetAfterComplete){
  time_t call_time = 0;
  timer.time([&call_time](const boost::system::error_code& ecode){
    std::cout << "handler1 called" << std::endl;
    EXPECT_EQ(boost::system::errc::success, ecode);
    call_time = time(NULL);
  });
  time_t start_time = time(NULL);
  std::thread t1([this](){ ios.run(); });
  sleep(20);
  timer.reset();
  t1.join();
  std::cout << "time difference: " << (call_time - start_time) << std::endl;
  EXPECT_TRUE((call_time - start_time) == 10);
}

