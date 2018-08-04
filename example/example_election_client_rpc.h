#ifndef EXAMPLE_ELECTION_CLIENT_RPC
#define EXAMPLE_ELECTION_CLIENT_RPC

#include <string>

#include <raft_util.h>
#include <example_election_util.h>

#include <boost/asio.hpp>

namespace ExampleClient {

class CRPC {
  boost::asio::io_service& mIos;

  template <typename H>
  void rpc_async_call_handler(Socket::ptr sock, const std::string& msg_obj, H handler, const boost::system::error_code& ec){
    using boost::asio::ip::tcp;
    namespace sev = boost::log::trivial;

    if (ec){
      BOOST_LOG_SEV(logger(), sev::error) << " failed at connect: " << ec.message();
      return;
    }

    boost::system::error_code ecode;
    std::string msg = ExampleUtil::build_str_msg(msg_obj);

    BOOST_LOG_SEV(logger(), sev::info) << "message sent to server: " << msg;

    boost::asio::write(sock->socket(), boost::asio::buffer(msg), ecode);

    if (ecode){
      BOOST_LOG_SEV(logger(), sev::error) << " failed at client write: " << ecode.message();
      return;
    }

    std::string rep_msg(ExampleUtil::MAX_MSG_SIZE, 0);
    sock->socket().read_some(boost::asio::buffer(rep_msg), ecode);

    if (ecode){
      BOOST_LOG_SEV(logger(), sev::error) << " failed at client read: " << ecode.message();
      return;
    }

    std::string rep_obj = ExampleUtil::read_str_msg(rep_msg);
    handler(rep_obj);
  }
public:
  CRPC(boost::asio::io_service& ios): mIos(ios) {}

  template <typename H>
  void rpc_async_call(const Address& addr, const std::string& msg_obj, H handler){
    namespace sev = boost::log::trivial;
    using boost::asio::ip::tcp;

    BOOST_LOG_SEV(logger(), sev::info) << "Call requested to " << addr.ip() << ":" << addr.port() << " msg: " << msg_obj;

    //TODO: this probably failed when servers are not online
    tcp::resolver resolver(mIos);
    tcp::resolver::query query(addr.ip().c_str(), addr.port().c_str());
    tcp::resolver::iterator epiter = resolver.resolve(query);
    Socket::ptr sock = Socket::create(mIos);

    boost::asio::async_connect(sock->socket(), epiter, std::bind(&CRPC::rpc_async_call_handler<H>, this, sock, msg_obj, handler, std::placeholders::_1));
  }
};

} //ExampleClient

#endif//EXAMPLE_ELECTION_CLIENT_RPC
