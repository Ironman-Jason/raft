#ifndef EXAMPLE_ELECTION_SERVER
#define EXAMPLE_ELECTION_SERVER

#include <example_election_server_rpc.h>
#include <raft_election_server.h>

#include <string>

#include <boost/asio.hpp>

namespace ExampleServer {

//example server that uses master election to select for single master
class ExampleElectionServer : public Raft::ElectionServer {
  std::string              mName;
  Address                  mAddress;
  ExampleServer::CRPC      mRPC;

  std::string client_request_handler(const std::string&){
    std::string ret;
    if (is_master())
      ret = "I am master";
    else
      ret = "I am slave";
    return ret;
  }

public:
  ExampleElectionServer(boost::asio::io_service& ios, const std::string& name, const Raft::ServerConfig& config, const Raft::RaftProperty& property):
    Raft::ElectionServer(ios, name, config, property),
    mName(name), mAddress(config.address.ip(), config.client_port),
    mRPC(ios, config.client_port) {
    mRPC.rpc_persistent_async_dispatch([this](const std::string& s){
      return client_request_handler(s);
    });
  }
};//ExampleElectionServer
} //ExampleServer

#endif//EXAMPLE_ELECTION_SERVER
