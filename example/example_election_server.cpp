#include <iostream>
#include <thread>

#include <raft_util.h>
#include <raft_config.h>

#include <example_election_server.h>

#include <boost/asio.hpp>

int main(int argc, const char* argv[]){
  namespace sev = boost::log::trivial;

  if (argc != 2){
    std::cerr << "Expected 1 parameter: [server_name]. exiting" << std::endl;
    exit(1);
  }

  std::string name = argv[1];

  Raft::RaftProperty raft_property = Raft::read_configuration("./example_election_server_config.json");
  const Raft::ServerConfig& config = raft_property.server_map[name.c_str()];

  init_log(config.logfile.c_str());

  BOOST_LOG_SEV(logger(), sev::info) << "server " << name << " started. logfile: " << config.logfile;

  boost::asio::io_service io;

  ExampleServer::ExampleElectionServer server(io, name, config, raft_property);

  std::thread t1([&io](){
      io.run();
      BOOST_LOG_SEV(logger(), sev::info) << "thread 1 exit";
  });
  std::thread t2([&io](){
      io.run();
      BOOST_LOG_SEV(logger(), sev::info) << "thread 2 exit";
  });
  std::thread t3([&io](){
      io.run();
      BOOST_LOG_SEV(logger(), sev::info) << "thread 3 exit";
  });
  std::thread t4([&io](){
      io.run();
      BOOST_LOG_SEV(logger(), sev::info) << "thread 4 exit";
  });

  t1.join();
  t2.join();
  t3.join();
  t4.join();
}
