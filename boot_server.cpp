//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <atomic>
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>

using boost::asio::ip::tcp;
time_t last;

std::atomic<uint64_t> total;
std::atomic<uint64_t> old;

class session : public std::enable_shared_from_this<session> {
public:
  session(tcp::socket socket) : socket_(std::move(socket)) {}

  void start() { do_read(); }

private:
  void do_read() {
    auto self(shared_from_this());
    boost::asio::async_read(
        socket_, boost::asio::buffer(data_, 9),
        [this, self](boost::system::error_code ec, std::size_t length) {
          if (length != 9) {
            std::cout << "length: " << length << "\n";
          }
          if (!ec) {
            // printf("%s %lu\n",data_,length);
            total.fetch_add(1);

            if (time(NULL) - last >= 1) {
              printf("qps: %lu\n", total - old);
              old = total.load();
              last = time(NULL);
            }
            do_read();
            do_write(length);
          }
        });
  }

  void do_write(std::size_t length) {
    auto self(shared_from_this());
    boost::asio::async_write(
        socket_, boost::asio::buffer(data_, length),
        [this, self](boost::system::error_code ec, std::size_t length) {
          if (!ec) {

            // std::cout << "Sent: " << data_ << "\n";
          }
        });
  }

  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
};

class server {
public:
  server(boost::asio::io_context &io_context, short port)
      : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
    do_accept();
  }

private:
  void do_accept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket) {
          if (!ec) {
            std::make_shared<session>(std::move(socket))->start();
          }

          do_accept();
        });
  }

  tcp::acceptor acceptor_;
};

int main(int argc, char *argv[]) {
  try {
    if (argc != 2) {
      std::cerr << "Usage: async_tcp_echo_server <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    server s(io_context, std::atoi(argv[1]));

    io_context.run();
  } catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}