#include <raft_util.h>
#include <raft_msg.h>
#include <raft_rpc.h>

#include <string>
#include <exception>
#include <optional>
#include <thread>
#include <random>
#include <iostream>

#include <boost/asio.hpp>

#include <gtest/gtest.h>

using boost::asio::ip::tcp;

TEST(BuildMsg, BuildRequestVote1){
  std::string t = Raft::build_msg<Raft::RequestVote>(1, "test1", 2, 3);
  for (size_t i = 0; i < t.length(); ++i){
    std::cout << (int)t[i] << " ";
  }
  std::cout << std::endl;
}

TEST(BuildMsg, BuildRequestVote2){
  Raft::RequestVote o(2, "test2", 3, 4);
  std::string t = Raft::build_msg(o);
  for (size_t i = 0; i < t.length(); ++i){
    std::cout << (int)t[i] << " ";
  }
  std::cout << std::endl;
}

TEST(BuildMsg, BuildAppendEntries1){
  std::string t = Raft::build_msg<Raft::AppendEntries<std::string>>(13, "test2", 9, 8, 7);
  for (size_t i = 0; i < t.length(); ++i)
    std::cout << (int)t[i] << " ";
  std::cout << std::endl;
}

TEST(BuildMsg, BuildAppendEntries2){
  Raft::AppendEntries<std::string> o(15, "test2", 17, 19, 22);
  o.push_back("lololo");
  o.push_back("hahaha");
  std::string t = Raft::build_msg(o);
  for (size_t i = 0; i < t.length(); ++i)
    std::cout << (int)t[i] << " ";
  std::cout << std::endl;
}

TEST(BuildMsg, BuildAppendEntriesLarge){
  Raft::AppendEntries<std::string> o(19, "test3", 1919, 1717, 77);
  for (size_t i = 0; i < 2000; ++i)
    o.push_back("wowowwo");
  std::string t = Raft::build_msg(o);
  std::cout << "length: " << t.length() << std::endl;
}

TEST(BuildMsg, BuildReply1){
  Raft::Reply r(8888, true);
  std::string t = Raft::build_msg(r);
  for (size_t i = 0; i < t.length(); ++i)
    std::cout << (int)t[i] << " ";
  std::cout << std::endl;
}

TEST(BuildMsg, BuildReply2){
  std::string t = Raft::build_msg<Raft::Reply>(9999, false);
  for (size_t i = 0; i < t.length(); ++i)
    std::cout << (int)t[i] << " ";
  std::cout << std::endl;
}

TEST(MsgType, ReadRequestVote1){
  Raft::RequestVote o(11, "test11", 15, 17);
  std::string s = Raft::build_msg(o);
  Raft::RaftMsgType t = Raft::msg_type(s);
  EXPECT_EQ(Raft::RVoteReq, t);
}

TEST(ReadMsg, ReadRequestVote1){
  Raft::RequestVote o(27, "test27", 97, 107);
  std::string s = Raft::build_msg(o);
  Raft::RequestVote t = Raft::read_msg_as<Raft::RequestVote>(s);
  EXPECT_EQ(27, t.term);
  EXPECT_STREQ("test27", t.candidateId.c_str());
  EXPECT_EQ(97, t.lastLogTerm);
  EXPECT_EQ(107, t.lastLogIndex);
}

TEST(ReadMsg, ReadAppendEntries1){
  Raft::AppendEntries<std::string> o(1919, "test77", 9999, 8888, 4786);
  std::string s = Raft::build_msg(o);
  Raft::RaftMsgType t = Raft::msg_type(s);
  EXPECT_EQ(Raft::AEntReq, t);
}

TEST(ReadMsg, ReadAppendEntries2){
  Raft::AppendEntries<std::string> o(6492, "test88", 4937, 2057, 9956);
  o.push_back("wala");
  o.push_back("steak");
  o.push_back("hamburger");
  std::string s = Raft::build_msg(o);
  Raft::AppendEntries<std::string> t = Raft::read_msg_as<Raft::AppendEntries<std::string>>(s);
  EXPECT_EQ(6492, t.term);
  EXPECT_STREQ("test88", t.leaderId.c_str());
  EXPECT_EQ(4937, t.prevLogTerm);
  EXPECT_EQ(2057, t.prevLogIndex);
  EXPECT_EQ(9956, t.leaderCommit);
  ASSERT_EQ(3, t.entries.size());
  EXPECT_STREQ("wala", t.entries[0].c_str());
  EXPECT_STREQ("steak", t.entries[1].c_str());
  EXPECT_STREQ("hamburger", t.entries[2].c_str());
}

TEST(ReadMsg, ReadReply1){
  Raft::Reply o(3865, true);
  std::string s = Raft::build_msg(o);
  Raft::RaftMsgType t = Raft::msg_type(s);
  EXPECT_EQ(Raft::Rep, t);
}

TEST(ReadMsg, ReadReply2){
  Raft::Reply o(4973, false);
  std::string s = Raft::build_msg(o);
  Raft::Reply t = Raft::read_msg_as<Raft::Reply>(s);
  EXPECT_EQ(4973, t.term);
  EXPECT_EQ(false, t.voteGranted);
}

TEST(TestFillContainer, TestFill1){
  Raft::AppendEntries<std::string> msgobj(47865, "test666", 97, 8476, 1222);
  std::vector<std::string> entries;
  for (size_t i = 0; i < 2000; ++i){
    entries.push_back("the entry");
  }
  size_t fill_size = Raft::fill_container(msgobj, entries.cbegin(), entries.cend());
  EXPECT_TRUE(fill_size < 2000);
  EXPECT_TRUE(msgobj.entries.size() == fill_size);

  std::string t = Raft::build_msg(msgobj);
  EXPECT_TRUE(t.length() < Raft::MAX_MSG_SZ);
}

TEST(TestFillContainer, TestRandomFill1){
  std::mt19937 eng;
  std::uniform_int_distribution<size_t> dist(1, 25);

  Raft::AppendEntries<std::string> msgobj(47865, "test666", 97, 8476, 1222);
  std::vector<std::string> entries;
  for (size_t i = 0; i < 2000; ++i){
    size_t len = dist(eng);
    std::string word;
    for (size_t j = 0; j < len; ++j){
      word += (char)(dist(eng) + 'a');
    }
    entries.push_back(word);
  }
  size_t fill_size = Raft::fill_container(msgobj, std::cbegin(entries), std::cend(entries));
  EXPECT_TRUE(fill_size < 2000);
  EXPECT_TRUE(msgobj.entries.size() == fill_size);

  std::string t = Raft::build_msg(msgobj);
  EXPECT_TRUE(t.length() < Raft::MAX_MSG_SZ);
}

class RPCCallTestService {
  boost::asio::io_service& mIos;
  tcp::acceptor            mAcceptor;
  Raft::RaftMsgType     mExpectedType;

  std::optional<std::string> read_msg(Socket::ptr& conn, boost::system::error_code& ecode){
    std::string msg(Raft::MAX_MSG_SZ, 0);
    conn->socket().read_some(boost::asio::buffer(msg), ecode);

    if (ecode) return std::nullopt;
    else       return msg;
  }

  void handle_accept(Socket::ptr conn, const boost::system::error_code&){
    boost::system::error_code ec;
    std::optional<std::string> recv_msg = read_msg(conn, ec);

    if (not recv_msg.has_value()){
      std::cout << "Bad input message received" << std::endl;
      accept();
      return;
    }

    Raft::RaftMsgType mtype = Raft::msg_type(recv_msg.value());

    ASSERT_EQ(mExpectedType, mtype);

    switch (mtype){
      case Raft::RVoteReq: {
        Raft::RequestVote msgobj = Raft::read_msg_as<Raft::RequestVote>(recv_msg.value());

        EXPECT_EQ(1337, msgobj.term);
        EXPECT_STREQ("leet", msgobj.candidateId.c_str());
        EXPECT_EQ(2664, msgobj.lastLogTerm);
        EXPECT_EQ(3993, msgobj.lastLogIndex);
      } break;
      case Raft::AEntReq: {
        Raft::AppendEntries<std::string> msgobj = Raft::read_msg_as<Raft::AppendEntries<std::string>>(recv_msg.value());

        EXPECT_EQ(962, msgobj.term);
        EXPECT_STREQ("ihen", msgobj.leaderId.c_str());
        EXPECT_EQ(3498, msgobj.prevLogTerm);
        EXPECT_EQ(284, msgobj.prevLogIndex);
        EXPECT_EQ(747, msgobj.leaderCommit);
        ASSERT_EQ(2, msgobj.entries.size());
        EXPECT_STREQ("world", msgobj.entries[0].c_str());
        EXPECT_STREQ("hello", msgobj.entries[1].c_str());
      } break;
      case Raft::InSnapReq: {
        Raft::InstallSnapshot msgobj = Raft::read_msg_as<Raft::InstallSnapshot>(recv_msg.value());

        EXPECT_EQ(3846, msgobj.term);
        EXPECT_STREQ("jihs", msgobj.leaderId.c_str());
        EXPECT_EQ(3736, msgobj.lastIncludedTerm);
        EXPECT_EQ(2937, msgobj.lastIncludedIndex);
        EXPECT_EQ(2847, msgobj.offset);
        EXPECT_EQ(true, msgobj.done);
      } break;
      default: assert(false); break;
    }

    Raft::Reply rep(1, true);
    std::string rep_msg = Raft::build_msg(rep);
    boost::asio::write(conn->socket(), boost::asio::buffer(rep_msg), ec);
  }
public:
  RPCCallTestService(boost::asio::io_service& ios, const std::string& port):
    mIos(ios), mAcceptor(ios, tcp::endpoint(tcp::v4(), std::stoi(port))), mExpectedType(Raft::RVoteReq) {}
  
  void accept(){
    Socket::ptr conn = Socket::create(mIos);
    mAcceptor.async_accept(conn->socket(), std::bind(&RPCCallTestService::handle_accept, this, conn, std::placeholders::_1));
  }

  void set_expected_type(Raft::RaftMsgType typ){
    mExpectedType = typ;
  }
};

struct TestRaftRPCCall : testing::Test {
  boost::asio::io_service ios;
  RPCCallTestService test_server;

  TestRaftRPCCall(): ios(), test_server(ios, "6666") {
    init_log("test_raft_rpc_call.log");
  }
  ~TestRaftRPCCall(){}
};

TEST_F(TestRaftRPCCall, testRequestVoteCall){
  try {
    test_server.set_expected_type(Raft::RVoteReq);
    test_server.accept();
    std::thread t([this](){ ios.run(); });
  
    Raft::RPC<std::string> rpc(ios, "6667");
    std::optional<Raft::Reply> rep = rpc.rpc_call<Raft::Reply>(Address("localhost", "6666"), Raft::RequestVote(1337, "leet", 2664, 3993));
    ASSERT_TRUE(rep.has_value());
    EXPECT_EQ(1, rep.value().term);
    EXPECT_EQ(true, rep.value().voteGranted);

    t.join();
  } catch (std::exception& e){
    std::cerr << "caught error: " << e.what() << std::endl;
    ASSERT_TRUE(false);
  }
}

TEST_F(TestRaftRPCCall, testAppendEntries){
  try {
    test_server.set_expected_type(Raft::AEntReq);
    test_server.accept();
    std::thread t([this](){ ios.run(); });

    Raft::RPC<std::string> rpc(ios, "6667");
    Raft::AppendEntries<std::string> callobj(962, "ihen", 3498, 284, 747);
    callobj.push_back("world");
    callobj.push_back("hello");
    std::optional<Raft::Reply> rep = rpc.rpc_call<Raft::Reply>(Address("localhost", "6666"), callobj);;
    ASSERT_TRUE(rep.has_value());
    EXPECT_EQ(1, rep.value().term);
    EXPECT_EQ(true, rep.value().voteGranted);

    t.join();
  } catch (std::exception& e){
    std::cerr << "caught error: " << e.what() << std::endl;
    ASSERT_TRUE(false);
  }
}

TEST(TestRaftRPCFailedCall, testNoServerCall){
  try {
    boost::asio::io_service ios;
    Raft::RPC<std::string> rpc(ios, "6667");
    std::optional<Raft::Reply> rep = rpc.rpc_call<Raft::Reply>(Address("localhost", "6666"), Raft::RequestVote(1337, "leet", 2664, 3993));
    EXPECT_FALSE(rep.has_value());
  } catch (std::exception& e){
    std::cerr << "Caught error: " << e.what() << std::endl;
    ASSERT_TRUE(false);
  }
}

TEST_F(TestRaftRPCCall, testRequestVoteAsync){
  try {
    test_server.set_expected_type(Raft::RVoteReq);
    test_server.accept();
    std::thread t([this](){ ios.run(); });

    Raft::RPC<std::string> rpc(ios, "6667");
    rpc.rpc_async_call<Raft::Reply>(Address("localhost", "6666"),
      Raft::RequestVote(1337, "leet", 2664, 3993),
      [](Raft::Reply& rep){
        EXPECT_EQ(1, rep.term);
        EXPECT_EQ(true, rep.voteGranted);
    });
    ios.run();

    t.join();
  } catch (std::exception& e){
    std::cerr << "caught error: " << e.what() << std::endl;
    ASSERT_TRUE(false);
  }
}

TEST_F(TestRaftRPCCall, testAppendEntriesAsync){
  try {
    test_server.set_expected_type(Raft::AEntReq);
    test_server.accept();
    std::thread t([this]{ ios.run(); });

    Raft::RPC<std::string> rpc(ios, "6667");
    Raft::AppendEntries<std::string> callobj(962, "ihen", 3498, 284, 747);
    callobj.push_back("world");
    callobj.push_back("hello");
    rpc.rpc_async_call<Raft::Reply>(Address("localhost", "6666"), callobj, [](Raft::Reply& rep){
        EXPECT_EQ(1, rep.term);
        EXPECT_EQ(true, rep.voteGranted);
    });

    ios.run();
    t.join();
  } catch (std::exception& e){
    std::cerr << "caught error: " << e.what() << std::endl;
    ASSERT_TRUE(false);
  }
}

TEST(TestRaftRPCFailedCall, testNoServerCallAsync){
  try {
    boost::asio::io_service ios;
    Raft::RPC<std::string> rpc(ios, "6667");
    rpc.rpc_async_call<Raft::Reply>(Address("localhost", "6666"), Raft::RequestVote(1337, "leet", 2664, 3993), [](Raft::Reply){
        ASSERT_FALSE(!!!"should never reach here");
    });

    ios.run();
  } catch (std::exception& e){
    std::cerr << "caught error: " << e.what() << std::endl;
    ASSERT_FALSE(false);
  }
}

struct TestRaftRPCDispatch : testing::Test {
  boost::asio::io_service ios;

  TestRaftRPCDispatch(): ios() {
    init_log("test_raft_rpc_dispatcher.log");
  }
  ~TestRaftRPCDispatch(){}
};

TEST_F(TestRaftRPCDispatch, testRequestVoteDispatch){
  try {
    Raft::RPC<std::string> rpc(ios, "6667");

    rpc.rpc_async_call<Raft::Reply>(Address("localhost", "6667"), Raft::RequestVote(4739, "jfieyu", 438, 947),
      [](Raft::Reply& rep){
        EXPECT_EQ(48, rep.term);
        EXPECT_EQ(false, rep.voteGranted);
    });
    std::thread t([this](){ ios.run(); });

    rpc.rpc_dispatch(
      [](Raft::RequestVote& msg){
        EXPECT_EQ(4739, msg.term);
        EXPECT_STREQ("jfieyu", msg.candidateId.c_str());
        EXPECT_EQ(438, msg.lastLogTerm);
        EXPECT_EQ(947, msg.lastLogIndex);

        Raft::Reply rep(48, false);
        return rep;
      },
      [](Raft::AppendEntries<std::string>&){
        EXPECT_TRUE(false);
        Raft::Reply rep(0, false);
        return rep;
      },
      [](Raft::InstallSnapshot&){
        EXPECT_TRUE(false);
        Raft::Reply rep(0, false);
        return rep;
      }
    );

    t.join();
  } catch (std::exception& e){
    std::cerr << "caught error: " << e.what() << std::endl;
    ASSERT_TRUE(false);
  }
}

TEST_F(TestRaftRPCDispatch, testAppendEntriesDispatch){
  try {
    Raft::RPC<std::string> rpc(ios, "6667");
    Raft::AppendEntries<std::string> msgobj(4738, "koh fs", 4947, 2283, 474);
    msgobj.push_back("dyepnud");
    msgobj.push_back("ddgdg");
    msgobj.push_back("cufy");
    rpc.rpc_async_call<Raft::Reply>(Address("localhost", "6667"), msgobj,
      [](Raft::Reply& rep){
        EXPECT_EQ(8, rep.term);
        EXPECT_EQ(true, rep.voteGranted);
    });
    std::thread t([this](){ ios.run(); });

    rpc.rpc_dispatch(
      [](Raft::RequestVote&){
        EXPECT_TRUE(false);
        Raft::Reply rep(48, false);
        return rep;
      },
      [](Raft::AppendEntries<std::string>& msg){
        EXPECT_EQ(4738, msg.term);
        EXPECT_STREQ("koh fs", msg.leaderId.c_str());
        EXPECT_EQ(4947, msg.prevLogTerm);
        EXPECT_EQ(2283, msg.prevLogIndex);
        EXPECT_EQ(474, msg.leaderCommit);
        EXPECT_EQ(3, msg.entries.size());
        EXPECT_STREQ("dyepnud", msg.entries[0].c_str());
        EXPECT_STREQ("ddgdg", msg.entries[1].c_str());
        EXPECT_STREQ("cufy", msg.entries[2].c_str());

        Raft::Reply rep(8, true);
        return rep;
      },
      [](Raft::InstallSnapshot&){
        EXPECT_TRUE(false);
        Raft::Reply rep(0, false);
        return rep;
      }
    );

    t.join();
  } catch (std::exception& e){
    std::cerr << "caught error: " << e.what() << std::endl;
    ASSERT_TRUE(false);
  }
}

TEST_F(TestRaftRPCDispatch, testRequestVoteDispatchAsync){
  try {
    Raft::RPC<std::string> rpc(ios, "6667");

    rpc.rpc_async_call<Raft::Reply>(Address("localhost", "6667"), Raft::RequestVote(211, "46cneu", 9474, 646),
      [](Raft::Reply& rep){
        EXPECT_EQ(4756, rep.term);
        EXPECT_EQ(true, rep.voteGranted);
    });
    std::thread t1([this](){ ios.run(); });

    rpc.rpc_async_dispatch(
      [](Raft::RequestVote& msg){
        EXPECT_EQ(211, msg.term);
        EXPECT_STREQ("46cneu", msg.candidateId.c_str());
        EXPECT_EQ(9474, msg.lastLogTerm);
        EXPECT_EQ(646, msg.lastLogIndex);

        Raft::Reply rep(4756, true);
        return rep;
      },
      [](Raft::AppendEntries<std::string>&){
        EXPECT_TRUE(false);
        Raft::Reply rep(0, false);
        return rep;
      },
      [](Raft::InstallSnapshot&){
        EXPECT_TRUE(false);
        Raft::Reply rep(0, false);
        return rep;
      }
    );
    std::thread t2([this](){ ios.run(); });

    t1.join();
    t2.join();
  } catch (std::exception& e){
    std::cerr << "caught error: " << e.what() << std::endl;
    ASSERT_TRUE(false);
  }
}

TEST_F(TestRaftRPCDispatch, testAppendEntriesDispatchAsync){
  try {
    Raft::RPC<std::string> rpc(ios, "6667");
    Raft::AppendEntries<std::string> msgobj(376291, "cbviufe8ygry2k", 3636, 9191, 6476);
    msgobj.push_back("cvuiehfb ");
    msgobj.push_back(" 9uh iui ");
    msgobj.push_back("78yubpyh2goei2h");
    msgobj.push_back("nvfi");
    rpc.rpc_async_call<Raft::Reply>(Address("localhost", "6667"), msgobj,
      [](Raft::Reply& rep){
        EXPECT_EQ(748786, rep.term);
        EXPECT_EQ(true, rep.voteGranted);
    });
    std::thread t1([this](){ ios.run(); });

    rpc.rpc_async_dispatch(
      [](Raft::RequestVote&){
        EXPECT_TRUE(false);
        Raft::Reply rep(48, false);
        return rep;
      },
      [](Raft::AppendEntries<std::string>& msg){
        EXPECT_EQ(376291, msg.term);
        EXPECT_STREQ("cbviufe8ygry2k", msg.leaderId.c_str());
        EXPECT_EQ(3636, msg.prevLogTerm);
        EXPECT_EQ(9191, msg.prevLogIndex);
        EXPECT_EQ(6476, msg.leaderCommit);
        EXPECT_EQ(4, msg.entries.size());
        EXPECT_STREQ("cvuiehfb ", msg.entries[0].c_str());
        EXPECT_STREQ(" 9uh iui ", msg.entries[1].c_str());
        EXPECT_STREQ("78yubpyh2goei2h", msg.entries[2].c_str());
        EXPECT_STREQ("nvfi", msg.entries[3].c_str());

        Raft::Reply rep(748786, true);
        return rep;
      },
      [](Raft::InstallSnapshot&){
        EXPECT_TRUE(false);
        Raft::Reply rep(0, false);
        return rep;
      }
    );
    std::thread t2([this](){ ios.run(); });

    t1.join();
    t2.join();
  } catch (std::exception& e){
    std::cerr << "caught error: " << e.what() << std::endl;
    ASSERT_TRUE(false);
  }
}
