#ifndef RAFT_CONFIG
#define RAFT_CONFIG

#include <cassert>
#include <string>
#include <map>
#include <fstream>

#include <raft_util.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

namespace Raft {

struct ServerConfig {
  Address address;
  std::string client_port;
  std::string statelog;
  std::string logfile;

  ServerConfig() = default;
  ServerConfig(const std::string& ip, const std::string& comm_port, const std::string& client_port, const std::string& sl, const std::string& log):
    address(ip, comm_port), client_port(client_port), statelog(sl), logfile(log) {}
};

using RaftServerMap = std::map<std::string, ServerConfig>;

struct RaftProperty {
  unsigned      election_timeout_min;
  unsigned      election_timeout_max;
  unsigned      heartbeat_interval;
  RaftServerMap server_map;
};

RaftProperty read_configuration(const char* cf){
  using namespace rapidjson;

  RaftProperty ret;

  static std::string config_file = cf;
  std::ifstream ifs(config_file.c_str());
  IStreamWrapper isw(ifs);

  Document doc;
  doc.ParseStream(isw);

  assert(doc.HasMember("config"));
  const Value& config = doc["config"];
  assert(config.HasMember("election_timeout_min"));
  assert(config.HasMember("election_timeout_max"));
  assert(config.HasMember("heartbeat_interval"));
  assert(config["election_timeout_min"].IsInt());
  assert(config["election_timeout_max"].IsInt());
  assert(config["heartbeat_interval"].IsInt());
  ret.election_timeout_min = config["election_timeout_min"].GetInt();
  ret.election_timeout_max = config["election_timeout_max"].GetInt();
  ret.heartbeat_interval = config["heartbeat_interval"].GetInt();

  assert(doc.HasMember("servers"));
  const Value& svs = doc["servers"];
  assert(svs.IsArray());
  for (size_t idx = 0; idx < svs.Size(); ++idx){
    assert(svs[idx].HasMember("name"));
    assert(svs[idx].HasMember("ip"));
    assert(svs[idx].HasMember("comm_port"));
    assert(svs[idx].HasMember("client_port"));
    assert(svs[idx].HasMember("statelog"));
    assert(svs[idx].HasMember("logfile"));
    assert(svs[idx]["name"].IsString());
    assert(svs[idx]["ip"].IsString());
    assert(svs[idx]["comm_port"].IsInt());
    assert(svs[idx]["client_port"].IsInt());
    assert(svs[idx]["statelog"].IsString());
    assert(svs[idx]["logfile"].IsString());

    ret.server_map.insert(std::make_pair(svs[idx]["name"].GetString(), ServerConfig(svs[idx]["ip"].GetString(), std::to_string(svs[idx]["comm_port"].GetInt()), std::to_string(svs[idx]["client_port"].GetInt()), svs[idx]["logfile"].GetString(), svs[idx]["logfile"].GetString())));
  }

  return ret;
}

} //Raft

#endif//RAFT_CONFIG
