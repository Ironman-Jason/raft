#ifndef EXAMPLE_ELECTION_CLIENT
#define EXAMPLE_ELECTION_CLIENT

#include <string>
#include <iostream>

#include <example_election_client_rpc.h>

namespace ExampleClient {

class ExampleElectionClient {
  ExampleClient::CRPC mRPC;
  std::string request;

  void get_label_reply_handler(const std::string& rep){
    std::cout << "Reply obtained: " << rep << std::endl;
  }
public:
  explicit ExampleElectionClient(boost::asio::io_service& ios): mRPC(ios), request("hello") {}

  void get_label(const Address& addr){
    mRPC.rpc_async_call(addr, request, [this](const std::string& s){
        get_label_reply_handler(s);
    });
  }
};

} //ExampleClient

#endif//EXAMPLE_ELECTION_CLIENT
