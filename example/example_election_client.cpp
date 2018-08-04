#include <vector>
#include <thread>
#include <iostream>

#include <raft_util.h>

#include <example_election_client.h>

#include <boost/asio.hpp>

int main(int argc, char* argv[]){
  namespace sev = boost::log::trivial;

  if (argc != 2){
    std::cout << "Usage: example_election_client [logfile]. exiting." << std::endl;
    exit(1);
  }

  const char* logfile = argv[1];
  init_log(logfile);

  BOOST_LOG_SEV(logger(), sev::info) << "Client started";

  std::vector<Address> addrs;
  addrs.emplace_back("127.0.0.1", "8765");
  addrs.emplace_back("127.0.0.1", "8766");
  addrs.emplace_back("127.0.0.1", "8767");
  addrs.emplace_back("127.0.0.1", "8768");
  addrs.emplace_back("127.0.0.1", "8769");

  boost::asio::io_service io;
  ExampleClient::ExampleElectionClient client(io);


  for (size_t i = 0; i < addrs.size(); ++i){
    client.get_label(addrs[i]);
  }

  std::thread t1([&io](){ io.run(); });
  t1.join();
}
