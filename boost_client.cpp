#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/write.hpp>

#include <functional>
#include <iostream>
#include <string>

using boost::asio::ip::tcp;
using std::placeholders::_1;
using std::placeholders::_2;

class client {
public:
  client(boost::asio::io_context &io_context) : socket_(io_context) {}

  // Called by the user of the client class to initiate the connection process.
  // The endpoints will have been obtained using a tcp::resolver.
  void start(tcp::resolver::results_type endpoints) {
    // Start the connect actor.
    endpoints_ = endpoints;
    start_connect(endpoints_.begin());
  }

  // This function terminates all the actors to shut down the connection. It
  // may be called by the user of the client class, or by the class itself in
  // response to graceful termination or an unrecoverable error.
  void stop() {
    stopped_ = true;
    boost::system::error_code ignored_error;
    socket_.close(ignored_error);
  }

private:
  void start_connect(tcp::resolver::results_type::iterator endpoint_iter) {
    if (endpoint_iter != endpoints_.end()) {
      std::cout << "Trying " << endpoint_iter->endpoint() << "...\n";

      // Start the asynchronous connect operation.
      socket_.async_connect(
          endpoint_iter->endpoint(),
          std::bind(&client::handle_connect, this, _1, endpoint_iter));
    } else {
      // There are no more endpoints to try. Shut down the client.
      stop();
    }
  }

  void handle_connect(const boost::system::error_code &error,
                      tcp::resolver::results_type::iterator endpoint_iter) {
    if (stopped_)
      return;

    // The async_connect() function automatically opens the socket at the start
    // of the asynchronous operation. If the socket is closed at this time then
    // the timeout handler must have run first.
    if (!socket_.is_open()) {
      std::cout << "Connect timed out\n";

      // Try the next available endpoint.
      start_connect(++endpoint_iter);
    }

    // Check if the connect operation failed before the deadline expired.
    else if (error) {
      std::cout << "Connect error: " << error.message() << "\n";

      // We need to close the socket used in the previous connection attempt
      // before starting a new one.
      socket_.close();

      // Try the next available endpoint.
      start_connect(++endpoint_iter);
    }

    // Otherwise we have successfully established a connection.
    else {
      std::cout << "Connected to " << endpoint_iter->endpoint() << "\n";

      // Start the input actor.
      start_read();

      // Start the heartbeat actor.
      send_to_server();
    }
  }

  char buff[64];

  void start_read() {
    boost::asio::async_read(socket_, boost::asio::buffer(buff, 9),
                            std::bind(&client::handle_read, this, _1, _2));
  }

  void handle_read(const boost::system::error_code &error, std::size_t n) {
    if (stopped_)
      return;

    if (!error) {
      if (n != 9) {
        std::cout << "length != 9: " << n << "\n";
      }
      send_to_server();
      start_read();
    } else {
      // std::cout << "Error on receive: " << error.message() << "\n";

      stop();
    }
  }

  void send_to_server() {
    if (stopped_)
      return;

    boost::asio::async_write(socket_, boost::asio::buffer("123456789", 9),
                             std::bind(&client::handle_write, this, _1));
  }

  void handle_write(const boost::system::error_code &error) {
    if (stopped_)
      return;

    if (!error) {

    } else {
      std::cout << "Error on heartbeat: " << error.message() << "\n";

      stop();
    }
  }

private:
  bool stopped_ = false;
  tcp::resolver::results_type endpoints_;
  tcp::socket socket_;
  std::string input_buffer_;
};

int main(int argc, char *argv[]) {
  try {
    if (argc != 3) {
      std::cerr << "Usage: client <host> <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;
    tcp::resolver r(io_context);
    client c(io_context);

    c.start(r.resolve(argv[1], argv[2]));

    io_context.run();
  } catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
