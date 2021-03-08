
/*

This file contains the functionality of the client itsself.
  - URI
  - Response
  - Connection 

I wanted to implement this as an header only library for simpler and more lightweight usage.

*/

#ifndef HTTP_CLIENT_HPP
#define HTTP_CLIENT_HPP

#define HTTP_NEWLINE "\r\n"
#define HTTP_SPACE " "
#define HTTP_HEADER_SEPARATOR ": "
#define HTTP_BLANK_LINE "\r\n\r\n"

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <cstring>
#include <sstream>
#include <chrono>
#include <typeinfo>
#include <json.hpp>


#include <asio.hpp>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>

#include "spdlog/spdlog.h"
#include "mime_types.hpp"
#include "HTTPResponseObject.pb.h"


using namespace std;
using namespace asio::ip;

using json = nlohmann::json;

// SPDLOG
auto spdlogger = spdlog::stdout_color_mt("http_client_logger");


// Function for shell commands.
string exec(const char* cmd) {
    array<char, 128> buffer;
    string result;
    unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}


class uriSplitter {
public:
  uriSplitter(string &str) : str(str), position(0){};

  string next(string search, bool returnTail = false) {
    size_t hit = str.find(search, position);
    if (hit == string::npos) {

      if (returnTail)
        return tail();
      else 
        return "";

    }

    size_t oldPosition = position;
    position = hit + search.length();

    return str.substr(oldPosition, hit - oldPosition);
  };

  string tail() {
    size_t oldPosition = position;
    position = str.length();
    return str.substr(oldPosition);
  };

private:
  string str;
  size_t position;
};


// @type: stringMap = map<string, string> (For more convenient use).
typedef map<string, string> stringMap;


/*
  @struct:
        A class for the simpler handling of the input URI
*/
struct URI {

  string requestURI = "";

  void parseParameters() {
    uriSplitter qt(querystring);
    do {
      string key = qt.next("=");

      if (key == "")
        break;

      parameters[key] = qt.next("&", true);
    } while (true);

  }

  string getRequestURI() {
    return requestURI;
  }


  /*
    @constructor:
            Splits the URI into smaller, understandable parts
            - is also able to parse params into a string map
  */
  URI(string input, bool shouldParseParameters = false) {

    //save the initial request URI to avoid redundant code

    requestURI = input;

    uriSplitter t = uriSplitter(input);
    protocol = t.next("://");

    string hostPortString = t.next("/");

    uriSplitter hostPort(hostPortString);

    host = hostPort.next(hostPortString[0] == '[' ? "]:" : ":", true);

    if (host[0] == '[')
      host = host.substr(1, host.size() - 1);

    port = hostPort.tail();

    address = t.next("?", true);

    if(address.find(".") < address.length()) {
      filename = address.substr(address.find("/") + 1, address[address.length() - 1]);
    }   

    querystring = t.next("#", true);

    

    hash = t.tail();

    if (shouldParseParameters)
      parseParameters();

  };

  string protocol, host, port, address, querystring, hash, filename;
  stringMap parameters;
};

// Function for key-checking in json.
bool exists(const json& j, const std::string& key)
{
    return j.find(key) != j.end();
}

/*
  @class: 
           Contains the key elements and functions of the client.
*/
class HTTPClient {
  public: 
    typedef enum {
      OPTIONS = 0,
      GET,
      POST,
      PUT,
      DELETE
    } HTTPMethod;

  static const char *methodTostring(HTTPMethod method) {
    const char *methods[] = {"OPTIONS", "GET", "POST", "PUT", 
                             "DELETE", NULL};
    return methods[method];
  };

  static HTTPMethod stringToMethod(string method) {
    map<string, HTTPMethod> methods;

    methods["GET"] = HTTPMethod::GET;
    methods["POST"] = HTTPMethod::POST;
    methods["PUT"] = HTTPMethod::PUT;
    methods["DELETE"] = HTTPMethod::DELETE;

    return methods[method];
  };


  /*
    @params: - method contains the requested HTTP-Method
             - uri contains the whole uri

    @functionality: 
             This function sends out the request.
               - uses buffered Reader 
               - CHECKS IF THE REQUEST IS VALID
    
    @return: 
            returns a HTTPResponse object.

  */
  static HTTPResponseObject::HTTPResponseObject request(HTTPMethod method, URI uri, json json_data, string filePath = "", string auth_data = "") {   

    // Defaulting uri port to 80 if there is not port given.
    if (uri.port == "")
      uri.port = "80";


    string host = uri.host;
    string proto = uri.protocol;
    string port_num = uri.port;

    // Create the HTTPResponse Object.
    HTTPResponseObject::HTTPResponseObject hro;

    hro.set_protocol(proto);

    string host_address;
    // Add the ":" only if the port number is not 80 (proprietary port number).
    if (port_num.compare("80") != 0) 
        host_address = host + ":" + port_num;
    else
        host_address = host;


    try {

      // Creating io_context.
      asio::io_context io_ctx;

      // Create a resolver that does the DNS-work and an results list of endpoints.
      asio::ip::tcp::resolver resolver(io_ctx.get_executor());
      asio::ip::basic_resolver_results endpoint = resolver.resolve(host, port_num);
    

      tcp::socket socket(io_ctx);
      asio::connect(socket, endpoint);
  
      /* 
        Build the request.  
        "Connection: close" header so that the
        server will close the socket after transmitting the response.
      */
      asio::streambuf request;
      std::ostream request_stream(&request);

      string request_str;

      if(exists(json_data, "username") && exists(json_data, "password")) {
        string username = json_data["username"];
        string password = json_data["password"];
        auth_data = username + ":" + password;
      }

      if((method == HTTPClient::POST || method == HTTPClient::PUT) && filePath.compare("") != 0) {
          // Using "cat" to get the files content.
          string cat_command = "cat "+ filePath;
          string file_content = exec(&cat_command[0]);
          auto file_length = file_content.length();

          MIMEType mime;
          string mime_type = string(mime.getMimeFromExtension(filePath));

          // Build a POST / PUT Request.
          request_str = string(methodTostring(method)) + string(" /") +
                        uri.address + ((uri.querystring == "") ? "" : "?") +
                        uri.querystring + " HTTP/1.1" HTTP_NEWLINE 
                        "Host: " + uri.host + HTTP_NEWLINE
                        "Content-Type: " + mime_type + HTTP_NEWLINE
                        "Accept: */*" HTTP_NEWLINE
                        "Authorization: Basic " + auth_data + HTTP_NEWLINE
                        "Content-Length: " + to_string(file_length) + HTTP_NEWLINE
                        "Connection: close" HTTP_NEWLINE HTTP_NEWLINE;

          cout << request_str;
          
          // Write the request and the file into the request stream.
          request_stream << request_str;
          request_stream << file_content;

      } else {

          if(string(methodTostring(method)).compare("POST") == 0 || 
             string(methodTostring(method)).compare("PUT") == 0) {
              spdlog::get("http_client_logger")->error("Too few Arguments for this type of request!");
              exit(-1);
          }

          // Build a GET / DELETE Request.
          request_str =   string(methodTostring(method)) + string(" /") +
                          uri.address + ((uri.querystring == "") ? "" : "?") +
                          uri.querystring + " HTTP/1.1" HTTP_NEWLINE 
                          "Host: " + uri.host + HTTP_NEWLINE
                          "Accept: */*" HTTP_NEWLINE
                          "Authorization: Basic " + auth_data + HTTP_NEWLINE
                          "Connection: close" HTTP_NEWLINE HTTP_NEWLINE;

          // Write the request into the request stream.
          request_stream << request_str;
      }

      spdlog::get("http_client_logger")->info("The requests body is: " + request_str);

      // Send the request.
      spdlog::get("http_client_logger")->info("Sending...");
      asio::write(socket, request);

      /* 
         Read the response status line. The response streambuf will automatically
         grow.
      */
      spdlog::get("http_client_logger")->info("Preparing to process the response...");
      asio::streambuf response;
      asio::read_until(socket, response, "\r\n");

      // Check that response is OK.
      istream response_stream(&response);

      string http_version;
      response_stream >> http_version;

      unsigned int status_code;
      response_stream >> status_code;

      string status_message;
      getline(response_stream, status_message);

      if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
          spdlog::get("http_client_logger")->warn("The response is invalid!");
          hro.set_success(false);
      }

      // Check if a file was requested, if so => save the file, else log into logfile.
      std::time_t result = std::time(nullptr);
      string timestamp = std::asctime(std::localtime(&result));
      string fname = uri.filename;

      // Create a new file to save the response in. 
      std::ofstream requested_file;
      bool is_new_file = false;


      if(fname.compare("") == 0 || method == HTTPClient::DELETE) {
        spdlog::get("http_client_logger")->info("The response is being saved into a log-file");
        requested_file.open("../doc/response_log/log-" + timestamp + ".txt");
      } else {
        spdlog::get("http_client_logger")->info("The response-file is being saved!");
        requested_file.open(fname);
        is_new_file = true;
      }

      if (status_code != 200) {
          requested_file << "Response returned with status code " << status_code << '\n';
          hro.set_statuscode(status_code);
      }

      // Read the response headers, which are terminated by a blank line.
      asio::read_until(socket, response, HTTP_BLANK_LINE);

      // Process the response headers.
      string header;
      while (getline(response_stream, header) && header != "\r")
      {
          if(is_new_file == false) {
            requested_file << header << "\n";
          }
          spdlog::get("http_client_logger")->info("The response header is: " + header);
      }

      // Spacing :).
      cout << "\n";


      // Write whatever content we already have to output.
      if (response.size() > 0) {
          requested_file << &response;
      }

       // Spacing :).
      cout << "\n";

      // Read until EOF, writing data to output as we go.
      while (asio::read(socket, response, asio::transfer_at_least(1))) {
            requested_file << &response;

      }

      requested_file.close();
      
      // Spacing :).
      cout << "\n";

    } catch(exception& e) {
        // Check if the EOF-Error was thrown.
        string error = e.what();
        if(error.compare("read: End of file") == 0) {
          // Successfully read.
          spdlog::get("http_client_logger")->warn(e.what());
          hro.set_success(true);

          spdlog::get("http_client_logger")->info("Sucessfully processed the response!");
        } else {
          spdlog::get("http_client_logger")->error(e.what());
          // Non successfull request.
          hro.set_success(false);

        }
        
    }
  
    return hro;

  }


};

#endif


