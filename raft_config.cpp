#include <raft_config.h>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

#include <fstream>

namespace RaftConfig {

RaftServerMap read_configuration(const char* config_file){
  using namespace rapidjson;

  RaftServerMap ret;

  static std::string config_file = config_file;
  std::ifstream ifs(config_file.c_str());
  IStreamWrapper isw(ifs);

  Document doc;
  doc.ParseStream(isw);

  assert(doc.HasMember("servers"));
  const Value& svs = doc["servers"];
  assert(svs.IsArray());
  for (size_t idx = 0; idx < svs.Size(); ++idx){
    assert(svs[idx].HasMember("name"));
    assert(svs[idx].HasMember("ip"));
    assert(svs[idx].HasMember("port"));
    assert(svs[idx]["name"].IsString());
    assert(svs[idx]["ip"].IsString());
    assert(svs[idx]["comm_port"].IsInt());
    assert(svs[idx]["client_port"].IsInt());
    assert(svs[idx]["logfile"].IsString());

    ret.insert(std::make_pair(svs[idx]["name"].GetString(), ServerConfig(svs[idx]["ip"].GetString(), svs[idx]["comm_port"].GetString(), svs[idx]["client_port"].GetString(), svs[idx]["logfile"].GetString())));
  }

  return ret;
}

} //RaftConfig
