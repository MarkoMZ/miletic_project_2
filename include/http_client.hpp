
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

#include <asio.hpp>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>


using namespace std;
using namespace asio::ip;

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


/*
  @class:
        class for the HTTPResponse
*/
class HTTPResponse {
  public: 
    bool success;
    string protocol;
    string response;
    string responseString;
    string statusCode;

    stringMap header;

    string body;

    HTTPResponse() : success(true) {};

    static HTTPResponse error() {
      HTTPResponse result;
      result.success = false;
      return result;
    }
};

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
  static HTTPResponse request(HTTPMethod method, URI uri) {

    cout << methodTostring(method) << '\n';

    cout << "requestURI: " << uri.getRequestURI() << '\n';
    cout << "Protocol: " << uri.protocol << '\n';
    cout << "Host : " << uri.host<< '\n';
    cout << "Port: " << uri.port << '\n';
    cout << "Address: " << uri.address << '\n';
    cout << "Filename: " << uri.filename << '\n';
    cout << "QueryString: " << uri.querystring << '\n';
    cout << "Hash: " << uri.hash << '\n';
    cout << "-----------------------------------" << endl;

    // Defaulting uri port to 80 if there is not port given.
    if (uri.port == "")
      uri.port = "80";


    string host = uri.host;
    string proto = uri.protocol;
    string port_num = uri.port;

    // Create the HTTPResponse Object
    HTTPResponse hr;

    string host_address;
    // Add the ":" only if the port number is not 80 (proprietary port number).
    if (port_num.compare("80") != 0) 
          host_address = host + ":" + port_num;
    else
        host_address = host;


    try {

      //Creating io_context
      asio::io_context io_ctx;

      //Create a resolver that does the DNS-work and an results list of endpoints.
      asio::ip::tcp::resolver resolver(io_ctx.get_executor());
      asio::ip::basic_resolver_results endpoint_iterator = resolver.resolve(host, port_num);
    

      tcp::socket socket(io_ctx);
      asio::connect(socket, endpoint_iterator);
  
      /* 
        Build the request.  
        "Connection: close" header so that the
        server will close the socket after transmitting the response.
      */
      asio::streambuf request;
      std::ostream request_stream(&request);

      string request_str = string(methodTostring(method)) + string(" /") +
                       uri.address + ((uri.querystring == "") ? "" : "?") +
                       uri.querystring + " HTTP/1.1" HTTP_NEWLINE "Host: " +
                       uri.host + HTTP_NEWLINE
                       "Accept: */*" HTTP_NEWLINE
                       "Connection: close" HTTP_NEWLINE HTTP_NEWLINE;

      request_stream << request_str;

      // Send the request.
      asio::write(socket, request);

      /* 
         Read the response status line. The response streambuf will automatically
         grow.
      */
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
          cout << "Invalid response" << '\n';
          hr.success = false;
      }
      if (status_code != 200) {
          cout << "Response returned with status code " << status_code << '\n';
          hr.statusCode = status_code;
      }

      // Read the response headers, which are terminated by a blank line.
      asio::read_until(socket, response, HTTP_BLANK_LINE);

      // Process the response headers.
      string header;
      while (getline(response_stream, header) && header != "\r")
      {
          cout << header << "\n";
      }

      // Spacing :).
      cout << "\n";

      // Create a new file to save the response in. (Download)
      string fname = uri.filename;
      std::ofstream requested_file(fname);

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
        cout << "Exception: " << e.what() << "\n";

        // Check if the EOF-Error was thrown.

        string error = e.what();
        if(error.compare("read: End of file") == 0) {
          // Successfully read.
          hr.success = true;
        } else {
          // Non successfull request.
          hr.success = false;
        }
        
    }
  
    return hr;

  }


};

#endif


