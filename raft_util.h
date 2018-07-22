#ifndef RAFT_UTIL
#define RAFT_UTIL

#include <string>
#include <memory>

#include <boost/asio.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

namespace log = boost::log;
using Int = unsigned long long;

class Socket: public std::enable_shared_from_this<Socket> {
  boost::asio::ip::tcp::socket mSocket;
public:
  Socket(boost::asio::io_service& ios): mSocket(ios) {}

  using ptr = std::shared_ptr<Socket>;

  static ptr create(boost::asio::io_service& ios){
    return ptr(new Socket(ios));
  }

  boost::asio::ip::tcp::socket& socket(){ return mSocket; }
};

class Address {
  std::string mIP;
  std::string mPort;
public:
  Address(const std::string& ip, const std::string& port): mIP(ip), mPort(port) {}
  Address(const std::string& ip, int port): mIP(ip), mPort(std::to_string(port)) {}
  Address() = default;
  ~Address() = default;

  const std::string& ip() const { return mIP; }
  const std::string& port() const { return mPort; }
  int portno() const { return std::stoi(mPort); }
};

void init_log(const char* filename){
  log::add_file_log(
      log::keywords::file_name = filename,
      log::keywords::format = "[%TimeStamp%]: %Message%"
  );
  log::core::get()->set_filter(log::trivial::severity >= log::trivial::info);
  log::add_common_attributes();
}

log::sources::severity_logger<log::trivial::severity_level>& logger(){
  static log::sources::severity_logger<log::trivial::severity_level> logger;
  return logger;
}

#endif//RAFT_UTIL
