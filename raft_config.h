#ifndef RAFT_CONFIG
#define RAFT_CONFIG

#include <string>
#include <map>

#include <raft_util.h>

namespace RaftConfig {

struct ServerConfig {
  Address address;
  std::string client_port;
  std::string logfile;

  ServerConfig() = default;
  ServerConfig(const std::string& ip, const std::string& comm_port, const std::string& client_port, const std::string& log):
    address(ip, comm_port), client_port(client_port), logfile(log) {}
  ~ServerConfig() = default;
};

using RaftServerMap = std::map<std::string, ServerConfig>;

RaftServerMap read_configuration(const char* config_file);

} //RaftConfig

#endif//RAFT_CONFIG
