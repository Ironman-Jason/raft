#ifndef EXAMPLE_ELECTION_SERVER_RPC
#define EXAMPLE_ELECTION_SERVER_RPC

#include <string>
#include <sstream>

#include <raft_util.h>
#include <example_election_util.h>

namespace ExampleServer {

class CRPC {
  boost::asio::io_service& mIos;
  boost::asio::ip::tcp::acceptor mAcceptor;

  template <typename H>
  void rpc_async_dispatch_handler_helper(Socket::ptr sock, H handler, const boost::system::error_code& ec){
    namespace sev = boost::log::trivial;

    if (ec){
      BOOST_LOG_SEV(logger(), sev::error) << " example client receiver failed at accept: " << ec.message();
      return;
    }

    boost::system::error_code ecode;
    std::string recv_msg(ExampleUtil::MAX_MSG_SIZE, 0);
    sock->socket().read_some(boost::asio::buffer(recv_msg), ecode);

    if (ecode){
      BOOST_LOG_SEV(logger(), sev::error) << " example client receiver failed at read_some: " << ecode.message();
      BOOST_LOG_SEV(logger(), sev::error) << " raw message: " << recv_msg;
      return;
    }

    std::string recv_msg_str = ExampleUtil::read_str_msg(recv_msg);

    BOOST_LOG_SEV(logger(), sev::info) << "message received: " << recv_msg_str;

    std::string rep_msg_str = handler(recv_msg_str);
    std::string rep_msg = ExampleUtil::build_str_msg(rep_msg_str);
    boost::asio::write(sock->socket(), boost::asio::buffer(rep_msg), ecode);

    if (ecode){
      BOOST_LOG_SEV(logger(), sev::error) << " example client receiver failed at write: " << ecode.message();
      return;
    }
  }

  template <typename H>
  void rpc_persistent_async_dispatch_handler(Socket::ptr sock, H handler, const boost::system::error_code& ecode){
    rpc_persistent_async_dispatch(handler);
    rpc_async_dispatch_handler_helper(sock, handler, ecode);
  }
public:
  CRPC(boost::asio::io_service& ios, const std::string& port):
    mIos(ios), mAcceptor(ios, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), std::stoi(port))) {}

  template <typename H>
  void rpc_persistent_async_dispatch(H handler){
    namespace sev = boost::log::trivial;

    BOOST_LOG_SEV(logger(), sev::info) << "Begin listening to client request";

    Socket::ptr sock = Socket::create(mIos);
    mAcceptor.async_accept(sock->socket(), std::bind(&CRPC::rpc_persistent_async_dispatch_handler<H>, this, sock, handler, std::placeholders::_1));
  }
};

}

#endif//EXAMPLE_ELCTION_SERVER_RPC
