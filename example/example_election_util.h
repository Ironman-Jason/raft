#ifndef EXAMPLE_ELECTION_UTIL
#define EXAMPLE_ELECTION_UTIL

#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/string.hpp>

namespace ExampleUtil {

constexpr size_t MAX_MSG_SIZE = 1500U;

std::string build_str_msg(const std::string& msg){
  std::stringstream msg_ss;
  {
    cereal::PortableBinaryOutputArchive oarchive(msg_ss);
    oarchive(msg);
  }
  return msg_ss.str();
}

std::string read_str_msg(const std::string& msg){
  std::stringstream rpc_ss; rpc_ss << msg;
  std::string ret;
  {
    cereal::PortableBinaryInputArchive iarchive(rpc_ss);
    iarchive(ret);
  }
  return ret;
}

} //ExampleUtil

#endif//EXAMPLE_ELECTION_UTIL
