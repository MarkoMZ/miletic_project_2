
/*

This file contains the functionality of the client itsself.
  - URL
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
#include "spdlog/sinks/stdout_color_sinks.h"


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
    unique_ptr<FILE, decltype(&pclose) > pipe(popen(cmd, "r"), pclose);
    if(!pipe) {
        throw runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

/*
  @class: urlSplitter should be used for tokenizing the passed URL.


*/
class urlSplitter {
public:
  urlSplitter(string &str) : str(str), position(0){};

  string next(string search, bool returnTail = false) {
    size_t hit = str.find(search, position);
    if(hit == string::npos) {

      if(returnTail)
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


// @type: stringMap = map<string, string> 
// (For more convenient use).
typedef map<string, string> stringMap;


/*
  @struct:
        A struct for the simpler handling of the input URL

  @functionality:
        Is able to parse the parameters into a stringMap 
          => type stringMap
*/
struct URL {

  public: 
    string protocol, host, port, 
          address, querystring, hash, filename;
    stringMap parameters;

    string requestURL = "";

    void parseParameters() {
      urlSplitter qt(querystring);
      do {
        string key = qt.next("=");

        if(key == "")
          break;

        parameters[key] = qt.next("&", true);
      } while (true);

    }

    string getRequestURL() {
      return requestURL;
    }




    /*
      @constructor:
              Splits the URL into smaller, understandable parts
              - is also able to parse params into a string map
    */
    URL(string input, bool shouldParseParameters = false) {

      //save the initial request URL to avoid redundant code.
      requestURL = input;


      // Create an instance of @class: urlSplitter.
      urlSplitter t = urlSplitter(input);

      // Get the protocol.
      protocol = t.next("://");

      // Get the host-port.
      string hostPortString = t.next("/");

      // Create another instance of @class: urlSplitter, 
      //  which makes it easier to handle.
      urlSplitter hostPort(hostPortString);

      host = hostPort.next(hostPortString[0] == '[' ? "]:" : ":", true);

      if(host[0] == '[')
        host = host.substr(1, host.size() - 1);

      // Save the port.
      port = hostPort.tail();

      // Get the address
      address = t.next("?", true);

      // Get (if existing) the name of the file.
      if(address.find(".") < address.length()) {
        filename = address.substr(address.find("/") + 1, 
                  address[address.length() - 1]);
      }   

      // Save the query.
      querystring = t.next("#", true);

      // Get the hash.
      hash = t.tail();

      if(shouldParseParameters)
        parseParameters();

    };

};

// Function for key-checking in json.
bool exists(const json& j, const std::string& key)
{
    return j.find(key) != j.end();
}

// Function for string checking.
bool hasEnding (string const &fullString, string const &ending) {
        if (fullString.length() >= ending.length()) {
            return (0 == fullString.compare(fullString.length() - ending.length(),
                                            ending.length(), ending));
        } else {
            return false;
        }
}  

/*
  @class: 
          Contains the key elements and functions of the client.
*/
class HTTPClient {
  private:
    // Type HTTPMethod => enum which contains the supported HTTP-Methods.
    typedef enum {
      OPTIONS = 0,
      GET,
      POST,
      PUT,
      DELETE
    } HTTPMethod;


  public: 

  // Parses HTTPMethod to std::string (returns const char*).
    static const char *methodTostring(HTTPMethod method) {
        const char *methods[] = {"OPTIONS", "GET", "POST", "PUT", 
                                "DELETE", NULL};
        return methods[method];
    };

    // Parses std::string to HTTPMethod.
    static HTTPMethod stringToMethod(string method) {
        map<string, HTTPMethod> methods;

        methods["GET"] = HTTPMethod::GET;
        methods["POST"] = HTTPMethod::POST;
        methods["PUT"] = HTTPMethod::PUT;
        methods["DELETE"] = HTTPMethod::DELETE;

        return methods[method];
    }; 


    /*
        @params: - method: contains the requested HTTP-Method
                - URL: contains the whole URL

        @functionality: 
                This function sends out the request.
                - CHECKS IF THE REQUEST IS VALID
        
        @return: 
                returns a HTTPResponse object.

    */
    static HTTPResponseObject::HTTPResponseObject 
        request(HTTPMethod method, URL URL, 
                json json_data, 
                string filePath = "", 
                string auth_data = "", 
                string save_dir = "", string cookie = "") {   

        
        // Defaulting URL port to 80 if there is not port given.
        if(URL.port == "")
            URL.port = "80";
        
        string host = URL.host;
        string proto = URL.protocol;
        string port_num = URL.port;


        // Create the HTTPResponse Object.
        HTTPResponseObject::HTTPResponseObject hro;

        hro.set_protocol(proto);

        try {

            // Creating io_context.
            asio::io_context io_ctx;

            // Create a resolver that does the DNS-work and an results list of endpoints.
            asio::ip::tcp::resolver resolver(io_ctx.get_executor());
            asio::ip::basic_resolver_results endpoint = 
            resolver.resolve(host, port_num);

            // Connect to the socket.
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

            // Check if the JSON contains user-authentication data.
            if(exists(json_data, "username") && exists(json_data, "password")) {
                string username = json_data["username"];
                string password = json_data["password"];
                auth_data = username + ":" + password;
            }

            // Check which method is being used.
            if((method == HTTPClient::POST || method == HTTPClient::PUT) 
                && filePath.compare("") != 0) {
                // Using "cat" to get the files content.
                string cat_command = "cat "+ filePath;
                string file_content = exec(&cat_command[0]);
                auto file_length = file_content.length();

                // Create instance of @class: MIMETYPE
                MIMEType mime;

                // Determine which MIME-type the file has.
                string mime_type = string(mime.getMimeFromExtension(filePath));

                // Build a POST / PUT Request.
                request_str = string(methodTostring(method)) + string(" /") +
                            URL.address + ((URL.querystring == "") ? "" : "?") +
                            URL.querystring + " HTTP/1.1" HTTP_NEWLINE 
                            "Host: " + URL.host + HTTP_NEWLINE
                            "Content-Type: " + mime_type + HTTP_NEWLINE
                            "Accept: */*" HTTP_NEWLINE
                            "Authorization: Basic " + auth_data + HTTP_NEWLINE
                            "Content-Length: " + to_string(file_length) + HTTP_NEWLINE
                            "Connection: close" HTTP_NEWLINE 
                            "Cookie: " + cookie + HTTP_NEWLINE HTTP_NEWLINE;

                cout << request_str;
                
                // Write the request and the file into the request stream.
                request_stream << request_str;
                request_stream << file_content;

            } else {

                if(string(methodTostring(method)).compare("POST") == 0 || 
                    string(methodTostring(method)).compare("PUT") == 0) {
                    // No file was specified.
                    spdlog::get("http_client_logger")
                    ->error("Too few Arguments for this type of request!");

                    spdlog::get("http_client_logger")
                    ->info("Please specify a file path : -f [absolute path to file] !");

                    exit(-1);
                }

                // Build a GET / DELETE Request.
                request_str =  string(methodTostring(method)) + string(" /") +
                                URL.address + ((URL.querystring == "") ? "" : "?") +
                                URL.querystring + " HTTP/1.1" HTTP_NEWLINE 
                                "Host: " + URL.host + HTTP_NEWLINE
                                "Accept: */*" HTTP_NEWLINE
                                "Authorization: Basic " + auth_data + HTTP_NEWLINE
                                "Connection: close" HTTP_NEWLINE
                                "Cookie: " + cookie + HTTP_NEWLINE HTTP_NEWLINE;

                // Write the request into the request stream.
                request_stream << request_str;
            }

            spdlog::get("http_client_logger")
            ->info("The requests body is: \n" + request_str);

            // Send the request.
            spdlog::get("http_client_logger")
            ->info("Sending...");

            asio::write(socket, request);

            /* 
                Read the response status line. The response streambuf will automatically
                grow.
            */
            spdlog::get("http_client_logger")
            ->info("Preparing to process the response...");

            asio::streambuf response;
            asio::read_until(socket, response, "\r\n");

            // Check that response is OK.
            istream response_stream(&response);

            // Save the http_version.
            string http_version;
            response_stream >> http_version;

            // Save the status_code.
            unsigned int status_code;
            response_stream >> status_code;

            // Save the status message.
            string status_message;
            getline(response_stream, status_message);

            // Only support HTTP.
            if(!response_stream || http_version.substr(0, 5) != "HTTP/") {
                spdlog::get("http_client_logger")
                ->warn("The response is invalid!");

                hro.set_success(false);
            }
            
            // Check if a file was requested, if so 
            // => save the file, else log into logfile.
            // => gen. timestamp.
            std::time_t result = std::time(nullptr);
            string timestamp = std::asctime(std::localtime(&result)); 
            

            // If a save directory was specified, add it to the file name
            string fname = URL.filename;

            // Create a new file to save the response in. 
            std::ofstream requested_file;
            bool is_new_file = false;
            bool is_log = false;

            // Only save a log-file if the HTTPMethod is DELETE OR if no file was sent.
            if(fname.compare("") == 0 || method == HTTPClient::DELETE) {
            spdlog::get("http_client_logger")
                ->info("The response is being saved into a log-file");

            fname = "../response_log/HTTPipe_log-" + timestamp + ".txt";
            is_log = true;
            
            } else {
                spdlog::get("http_client_logger")
                    ->info("The response-file is being saved!");
                
                is_new_file = true;
            }

            // Always save the header.
            if(save_dir.compare("") != 0 && is_log) {
                fname = save_dir + "/" + "HTTPipe_log-" + timestamp + ".txt";
                spdlog::get("http_client_logger")
                    ->info("The response header is being saved into a log file");

            } else if(save_dir.compare("") != 0 && !is_log) {
                // Check if the path is ending with "/" or not
                if(hasEnding(save_dir, "/")) {
                    fname = save_dir + URL.filename;
                } else {
                    fname = save_dir + "/" + URL.filename;
                }

        
            }


            hro.set_savepath(fname);

            requested_file.open(fname);

            if(status_code != 200) {

                spdlog::get("http_client_logger")
                    ->info("Response returned with status code: {}", status_code);

                requested_file << "Response returned with status code: " 
                                << status_code  << '\n';

            } else {
                spdlog::get("http_client_logger")
                    ->info("Response returned with status code: 200");
                }

                hro.set_statuscode(status_code);

                // Read the response headers, which are terminated by a blank line.
                asio::read_until(socket, response, HTTP_BLANK_LINE);

                spdlog::get("http_client_logger")
                    ->info("A file is being saved to: {}", fname);

                // Process the response headers.
                string header;
                while (getline(response_stream, header) && header != "\r")
                {
                    if(is_new_file == false) {
                    requested_file << header << "\n";
                    }
                }

                    

                // Write whatever content we already have to output.
                if(response.size() > 0) {
                    requested_file << &response;
                }

                // Read until EOF, writing data to output as we go.
                while (asio::read(socket, response, asio::transfer_at_least(1))) {
                    requested_file << &response;

                }

                requested_file.close();

        } catch(exception& e) {
            // Check if the EOF-Error was thrown.
            string error = e.what();
            if(error.compare("read: End of file") == 0) {
                // Successfully read.
                spdlog::get("http_client_logger")
                    ->warn(e.what());

                hro.set_success(true);

                spdlog::get("http_client_logger")
                    ->info("Sucessfully processed the response!");

            } else {
                spdlog::get("http_client_logger")->error(e.what());
                // Non successfull request.
                hro.set_success(false);

            }
            
        }

        // Return the HTTPResponseObject.
        return hro;

    }

};

#endif


