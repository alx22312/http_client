#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

using boost::asio::ip::tcp;

enum { max_length = 1024 };

class client
{
public:
  client(boost::asio::io_service& io_service,
      const std::string& server,const std::string& port, const std::string& path)
    : resolver_(io_service),
      socket_(io_service),
      path_(path),
      server_(server)
  {
    // Form the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
/*    std::string path_;
    std::ostream request_stream(&request_);
    if (path == "")
    {
        char request[max_length];
        std::cout << "Enter message: ";
        std::cin.getline(request, max_length);
        sprintf(request, "%s\n",request);
        std::stringstream ss;
        ss << request;
        ss >> path_;
    }
    else
        path_ = path;
    request_stream << "GET " << path_ << " HTTP/1.0\r\n";
    request_stream << "Host: " << server << "\r\n";*/
    //request_stream << "Accept: */*\r\n";
    /*request_stream << "Connection: close\r\n\r\n";*/
    //path_ = path;
    set_request(true);

    // Start an asynchronous resolve to translate the server and service names
    // into a list of endpoints.
    tcp::resolver::query query(tcp::v4(),server, port);
    resolver_.async_resolve(query,
        boost::bind(&client::handle_resolve, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::iterator));
  }

private:

  void set_request(bool first)
  {
    char path_tmp[max_length];
    //std::ostream request_stream(&request_);

    std::cout << "request_ " <<  request_ <<"\n";
    char request[max_length];
    if ((path_ == "")||(!first))
    {
        std::cout << "Enter message: ";
        std::cin.getline(request, max_length);
//        sprintf(request, "%s\n",request);
//        std::stringstream ss;
//        ss << request;
//        ss >> path_tmp;
    }
    else
        strcpy(path_tmp, path_.c_str());
        //path_tmp = path_;
        sprintf(request, "GET %s HTTP/1.0\r\n",path_tmp);
        sprintf(request, "%sHost: %s\r\n",request,/*server_.c_str()*/"localhost");
        sprintf(request, "%sAccept: */*\r\n",request);
        sprintf(request, "%sConnection: close\r\n\r\n",request);
        //clear
        memset(request_, 0, sizeof(request_));
        //write new
        sprintf(request_, "%s",request);
//    request_stream << "GET " << path_tmp << " HTTP/1.0\r\n";
//    request_stream << "Host: " << server_ << "\r\n";
//    request_stream << "Accept: */*\r\n";
//    request_stream << "Connection: close\r\n\r\n";
  }

  void handle_resolve(const boost::system::error_code& err,
      tcp::resolver::iterator endpoint_iterator)
  {
    if (!err)
    {
      // Attempt a connection to each endpoint in the list until we
      // successfully establish a connection.
      boost::asio::async_connect(socket_, endpoint_iterator,
          boost::bind(&client::handle_connect, this,
            boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "Error: " << err.message() << "\n";
    }
  }

  void handle_connect(const boost::system::error_code& err)
  {
    if (!err)
    {
      // The connection was successful. Send the request.
      boost::asio::async_write(socket_, boost::asio::buffer(request_),
          boost::bind(&client::handle_write_request, this,
            boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "Error: " << err.message() << "\n";
    }
  }

  void handle_write_request(const boost::system::error_code& err)
  {
    if (!err)
    {
      // Read the response status line. The response_ streambuf will
      // automatically grow to accommodate the entire line. The growth may be
      // limited by passing a maximum size to the streambuf constructor.
      boost::asio::async_read_until(socket_, response_, "\r\n",
          boost::bind(&client::handle_read_status_line, this,
            boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "Error: " << err.message() << "\n";
    }
  }

  void handle_read_status_line(const boost::system::error_code& err)
  {
    if (!err)
    {
      // Check that response is OK.
      std::istream response_stream(&response_);
      std::string http_version;
      response_stream >> http_version;
      unsigned int status_code;
      response_stream >> status_code;
      std::string status_message;
      std::getline(response_stream, status_message);
      if (!response_stream || http_version.substr(0, 5) != "HTTP/")
      {
        std::cout << "Invalid response\n";
        return;
      }
      if (status_code != 200)
      {
        std::cout << "Response returned with status code ";
        std::cout << status_code << "\n";
        return;
      }

      // Read the response headers, which are terminated by a blank line.
      boost::asio::async_read_until(socket_, response_, "\r\n\r\n",
          boost::bind(&client::handle_read_headers, this,
            boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "Error: " << err << "\n";
    }
  }

  void handle_read_headers(const boost::system::error_code& err)
  {
    if (!err)
    {
      // Process the response headers.
      std::istream response_stream(&response_);
      std::string header;
      while (std::getline(response_stream, header) && header != "\r")
        std::cout << header << "\n";
      std::cout << "\n";

      // Write whatever content we already have to output.
      if (response_.size() > 0)
        std::cout << &response_;
      //New
      set_request(false);
//      boost::asio::async_write(socket_, request_,
//       boost::bind(&client::handle_write_request, this,
//         boost::asio::placeholders::error));
      // Start reading remaining data until EOF.
      boost::asio::async_write(socket_, boost::asio::buffer(request_),
          boost::bind(&client::handle_write_request, this,
            boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "Error: " << err << "\n";
    }
  }

  void handle_read_content(const boost::system::error_code& err)
  {
    if (!err)
    {
      // Write all of the data that has been read so far.
      std::cout << &response_;

      // Continue reading remaining data until EOF.
      boost::asio::async_read(socket_, response_,
          boost::asio::transfer_at_least(1),
          boost::bind(&client::handle_read_content, this,
            boost::asio::placeholders::error));
    }
    else if (err != boost::asio::error::eof)
    {
      std::cout << "Error: " << err << "\n";
    }
  }

  tcp::resolver resolver_;
  tcp::socket socket_;
  //boost::asio::streambuf request_;
  char request_[max_length];
  boost::asio::streambuf response_;
  const std::string& path_;
  const std::string& server_;
};

int main(int argc, char* argv[])
{
  try
  {
    setlocale(LC_CTYPE, "rus"); // вызов функции настройки локали
    char *ip = "127.0.0.1";
    char *def_port = "2015";
    char *start_command = "";
    if(argv[1] != NULL)
    {
        def_port = argv[1];
        if(argv[2] != NULL)
            def_port = argv[2];
         if(argv[3] != NULL)
            start_command = argv[3];
    }
    boost::asio::io_service io_service;
    client c(io_service, ip,def_port, start_command);
    //load cycle
    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cout << "Exception: " << e.what() << "\n";
  }

  return 0;
}
