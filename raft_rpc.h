#ifndef RAFT_RPC
#define RAFT_RPC

#include <string>
#include <sstream>
#include <optional>
#include <functional>

#include <boost/asio.hpp>

#include <raft_util.h>
#include <raft_config.h>
#include <raft_msg.h>

#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>

//for sending and receiving raft messages
namespace Raft {

constexpr size_t MAX_MSG_SZ = 1500U;

bool is_msg_sz_limit_exceeded(const std::string& msg){
  return not (msg.length() < MAX_MSG_SZ);
}

template <typename T, typename... Args>
std::string build_msg(const Args&... args){
  std::stringstream msg_ss;
  {
    cereal::PortableBinaryOutputArchive oarchive(msg_ss);
    Type type(T::etype());
    T msg(args...);
    oarchive(type);
    oarchive(msg);
  }
  return msg_ss.str();
}

template <typename T>
std::string build_msg(const T& obj){
  std::stringstream msg_ss;
  {
    cereal::PortableBinaryOutputArchive oarchive(msg_ss);
    Type type(T::etype());
    oarchive(type);
    oarchive(obj);
  }
  return msg_ss.str();
}

template <template <typename> class C, typename IT, typename E>
size_t fill_container(C<E>& msg_obj, IT beg, IT end){
  std::stringstream container_msg;
  {
    cereal::PortableBinaryOutputArchive oarchive(container_msg);
    Type type(C<E>::etype());
    oarchive(type);
    oarchive(msg_obj);
  }
  size_t container_sz = container_msg.str().length();

  size_t elems_filled = 0;
  size_t cur_sz = container_sz;
  for (IT it = beg; it != end; ++it){
    std::stringstream elem_ss;
    {
      cereal::PortableBinaryOutputArchive oarchive(elem_ss);
      oarchive(*it);
    }
    size_t elem_sz = elem_ss.str().length();

    if (elem_sz + cur_sz >= MAX_MSG_SZ) break;

    msg_obj.push_back(*it);
    cur_sz += elem_sz;
    elems_filled++;
  }
  return elems_filled;
}

RaftMsgType msg_type(const std::string& msg){
  std::stringstream rpc_ss; rpc_ss << msg;
  Type type;
  {
    cereal::PortableBinaryInputArchive archive(rpc_ss);
    archive(type);
  }
  return type.type;
}

template <typename T>
T read_msg_as(const std::string& msg){
  std::stringstream rpc_ss; rpc_ss << msg;
  Type type;
  T obj;
  {
    cereal::PortableBinaryInputArchive archive(rpc_ss);
    archive(type);
    archive(obj);
  }
  return obj;
}

template <typename T>
size_t msg_sz(const T& obj){
  std::stringstream msg_ss;
  {
    cereal::PortableBinaryOutputArchive oarchive(msg_ss);
    oarchive(obj);
  }
  return msg_ss.str().length();
}

template <typename E>
class RPC {
  boost::asio::io_service&       mIos;
  boost::asio::ip::tcp::acceptor mAcceptor;

  template <typename R, typename S, typename RHandler>
  void rpc_async_call_handler(Socket::ptr sock, const S& msg_obj, RHandler handler, const boost::system::error_code& ec){
    using boost::asio::ip::tcp;
    namespace sev = boost::log::trivial;

    if (ec){
      BOOST_LOG_SEV(logger(), sev::error) << __FUNCTION__ << " connect error: " << ec.message();
      return;
    }

    boost::system::error_code ecode;
    std::string msg = build_msg(msg_obj);

    if (is_msg_sz_limit_exceeded(msg)){
      BOOST_LOG_SEV(logger(), sev::error) << __FUNCTION__ << " RPC message size too big. refuse to send";
      return;
    }

    boost::asio::write(sock->socket(), boost::asio::buffer(msg), ecode);

    if (ecode){
      BOOST_LOG_SEV(logger(), sev::error) << __FUNCTION__ << " write error: " << ecode.message();
      return;
    }

    std::string rep_msg(MAX_MSG_SZ, 0);
    sock->socket().read_some(boost::asio::buffer(rep_msg), ecode);

    if (ecode){
      BOOST_LOG_SEV(logger(), sev::error) << __FUNCTION__ << " read error: " << ecode.message();
      return;
    }

    RaftMsgType rep_type = msg_type(rep_msg);

    if (rep_type != R::etype()){
      BOOST_LOG_SEV(logger(), sev::error) << __FUNCTION__ << " unexpected return object. could be corrupted. type " << rep_type;
      return;
    }

    R rep_obj = read_msg_as<R>(rep_msg);
    handler(rep_obj);
  }

  template <typename RVH, typename AEH, typename ISH>
  void rpc_async_dispatch_handler_helper(Socket::ptr& sock, RVH rvh, AEH aeh, ISH ish, const boost::system::error_code& ec){
    namespace sev = boost::log::trivial;

    if (ec){
      BOOST_LOG_SEV(logger(), sev::error) << __FUNCTION__ << " accept failed: " << ec.message();
      return;
    }

    boost::system::error_code ecode;
    std::string recv_msg(MAX_MSG_SZ, 0);
    sock->socket().read_some(boost::asio::buffer(recv_msg), ecode);

    if (ecode){
      BOOST_LOG_SEV(logger(), sev::error) << __FUNCTION__ << " read failed: " << ecode.message();
      return;
    }

    RaftMsgType mtype = msg_type(recv_msg);
    Reply rep;
    switch (mtype){
      case RVoteReq: {
        RequestVote msgobj = read_msg_as<RequestVote>(recv_msg);
        rep = rvh(msgobj);
      } break;
      case AEntReq: {
        AppendEntries<E> msgobj = read_msg_as<AppendEntries<E>>(recv_msg);
        rep = aeh(msgobj);
      } break;
      case InSnapReq: {
        //TODO: is Reply for InstallSnapshot correct?
        InstallSnapshot msgobj = read_msg_as<InstallSnapshot>(recv_msg);
        rep = ish(msgobj);
      } break;
      default: {
        BOOST_LOG_SEV(logger(), sev::error) << __FUNCTION__ << "received unexpected message type. message cannot be handled.";
        return;
      } break;
    }

    std::string rep_msg = build_msg(rep);
    boost::asio::write(sock->socket(), boost::asio::buffer(rep_msg), ecode);

    if (ecode){
      BOOST_LOG_SEV(logger(), sev::error) << __FUNCTION__ << " write failed: " << ecode.message();
      return;
    }
  }

  template <typename RVH, typename AEH, typename ISH>
  void rpc_async_dispatch_handler(Socket::ptr sock, RVH rvh, AEH aeh, ISH ish, const boost::system::error_code& ec){
    rpc_async_dispatch_handler_helper(sock, rvh, aeh, ish, ec);
  }

  template <typename RVH, typename AEH, typename ISH>
  void rpc_persistent_async_dispatch_handler(Socket::ptr sock, RVH rvh, AEH aeh, ISH ish, const boost::system::error_code& ec){
    //allow immediate readiness to accept additional connections if exist
    rpc_persistent_async_dispatch(rvh, aeh, ish);
    rpc_async_dispatch_handler_helper(sock, rvh, aeh, ish, ec);
  }

public:
  RPC(boost::asio::io_service& ios, const std::string& port):
    mIos(ios), mAcceptor(ios, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), std::stoi(port))) {}

  template <typename R, typename S>
  std::optional<R> rpc_call(const Address& addr, const S& msg_obj){
    using boost::asio::ip::tcp;
    namespace sev = boost::log::trivial;

    std::string msg = build_msg(msg_obj);

    if (is_msg_sz_limit_exceeded(msg)){
      BOOST_LOG_SEV(logger(), sev::error) << __FUNCTION__ << " message size too big. refuse to send";
      return std::nullopt;
    }

    BOOST_LOG_SEV(logger(), sev::info) << __FUNCTION__ << " sending rpc call";

    //TODO: evaluate if this has to be done every time
    tcp::resolver resolver(mIos);
    tcp::resolver::query query(addr.ip().c_str(), addr.port().c_str());
    tcp::resolver::iterator epiter = resolver.resolve(query);
    tcp::socket sock(mIos);
    boost::system::error_code ecode;

    boost::asio::connect(sock, epiter, ecode);

    if (ecode){
      BOOST_LOG_SEV(logger(), sev::error) << __FUNCTION__ << " connect failed: " << ecode.message();
      return std::nullopt;
    }

    boost::asio::write(sock, boost::asio::buffer(msg), ecode);

    if (ecode){
      BOOST_LOG_SEV(logger(), sev::error) << __FUNCTION__ << " write failed: " << ecode.message();
      return std::nullopt;
    }

    std::string rep_msg(MAX_MSG_SZ, 0);
    sock.read_some(boost::asio::buffer(rep_msg), ecode);

    if (ecode){
      BOOST_LOG_SEV(logger(), sev::error) << __FUNCTION__ << " read failed: " << ecode.message();
      return std::nullopt;
    }

    RaftMsgType rep_type = msg_type(rep_msg);

    if (rep_type != R::etype()) return std::nullopt;

    return read_msg_as<R>(rep_msg);
  }

  template <typename R, typename S, typename RHandler>
  void rpc_async_call(const Address& addr, const S& msg_obj, RHandler handler){
    namespace sev = boost::log::trivial;
    using boost::asio::ip::tcp;

    BOOST_LOG_SEV(logger(), sev::info) << __FUNCTION__ << " sending async rpc call";

    //TODO: evaluate if this has to be done every time
    tcp::resolver resolver(mIos);
    tcp::resolver::query query(addr.ip().c_str(), addr.port().c_str());
    tcp::resolver::iterator epiter = resolver.resolve(query);
    Socket::ptr sock = Socket::create(mIos);

    boost::asio::async_connect(sock->socket(), epiter, std::bind(&RPC::rpc_async_call_handler<R, S, RHandler>, this, sock, msg_obj, handler, std::placeholders::_1));
  }

  template <typename R, typename S>
  std::vector<std::optional<R>> rpc_broadcast(const std::vector<Address>& addrs, const S& msg_obj){
    namespace sev = boost::log::trivial;

    BOOST_LOG_SEV(logger(), sev::info) << "Begin broadcast";

    std::vector<std::optional<R>> ret;
    ret.reserve(addrs.size());
    for (const Address& addr : addrs)
      ret.push_back(rpc_call<R>(addr, msg_obj));
    return ret;
  }

  template <typename R, typename S, typename RHandler>
  void rpc_async_broadcast(const std::vector<Address>& addrs, const S& msg_obj, RHandler handler){
    namespace sev = boost::log::trivial;

    BOOST_LOG_SEV(logger(), sev::info) << "Begin broadcast.";

    for (const Address& addr : addrs)
      rpc_async_call<R>(addr, msg_obj, handler);
  }

  //handle one RPC dispatch
  template <typename RVHandler, typename AEHandler, typename ISHandler>
  void rpc_dispatch(RVHandler rvh, AEHandler aeh, ISHandler ish){
    using boost::asio::ip::tcp;
    namespace sev = boost::log::trivial;

    boost::system::error_code ecode;

    tcp::socket sock(mIos);
    mAcceptor.accept(sock, ecode);

    if (ecode){
      BOOST_LOG_SEV(logger(), sev::error) << __FUNCTION__ << " accept failed: " << ecode.message();
      return;
    }

    std::string recv_msg(MAX_MSG_SZ, 0);
    sock.read_some(boost::asio::buffer(recv_msg), ecode);

    if (ecode){
      BOOST_LOG_SEV(logger(), sev::error) << __FUNCTION__ << " read failed: " << ecode.message();
      return;
    }

    RaftMsgType mtype = msg_type(recv_msg);
    Reply rep;

    switch (mtype){
      case RVoteReq: {
        RequestVote rv = read_msg_as<RequestVote>(recv_msg);
        rep = rvh(rv);
      } break;
      case AEntReq: {
        AppendEntries<E> ae = read_msg_as<AppendEntries<E>>(recv_msg);
        rep = aeh(ae);
      } break;
      case InSnapReq: {
        //TODO: is Reply for InstallSnapshot correct?
        InstallSnapshot is = read_msg_as<InstallSnapshot>(recv_msg);
        rep = ish(is);
      } break;
      default: {
        BOOST_LOG_SEV(logger(), sev::error) << __FUNCTION__ << " message of unknown type received. refuse to handle.";
        return;
      } break;
    }

    std::string rep_msg = build_msg(rep);
    boost::asio::write(sock, boost::asio::buffer(rep_msg), ecode);

    if (ecode){
      BOOST_LOG_SEV(logger(), sev::error) << __FUNCTION__  << " write failed: " << ecode.message();
      return;
    }
  }

  template <typename RVHandler, typename AEHandler, typename ISHandler>
  [[ noreturn ]]
  void rpc_persistent_dispatch(RVHandler rvh, AEHandler aeh, ISHandler ish){
    namespace sev = boost::log::trivial;

    BOOST_LOG_SEV(logger(), sev::info) << "Begin listening to request";

    for (;;){
      rpc_dispatch(rvh, aeh, ish);
    }
  }

  template <typename RVH, typename AEH, typename ISH>
  void rpc_async_dispatch(RVH rvh, AEH aeh, ISH ish){
    namespace sev = boost::log::trivial;

    BOOST_LOG_SEV(logger(), sev::info) << "Begin listening to single request";

    Socket::ptr sock = Socket::create(mIos);
    mAcceptor.async_accept(sock->socket(), std::bind(&RPC::rpc_async_dispatch_handler<RVH, AEH, ISH>, this, sock, rvh, aeh, ish, std::placeholders::_1));
  }

  template <typename RVH, typename AEH, typename ISH>
  void rpc_persistent_async_dispatch(RVH rvh, AEH aeh, ISH ish){
    namespace sev = boost::log::trivial;

    BOOST_LOG_SEV(logger(), sev::info) << "Begin listening to request";

    Socket::ptr sock = Socket::create(mIos);
    mAcceptor.async_accept(sock->socket(), std::bind(&RPC::rpc_persistent_async_dispatch_handler<RVH, AEH, ISH>, this, sock, rvh, aeh, ish, std::placeholders::_1));
  }
};

} //Raft

#endif//RAFT_RPC
