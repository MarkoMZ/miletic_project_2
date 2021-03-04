
/*

This file contains the functionality of the client itsself.
  - URI
  - Response
  - Connection 

*/

#define HTTP_NEWLINE "\r\n"
#define HTTP_SPACE " "
#define HTTP_HEADER_SEPARATOR ": "

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <cstring>
#include <sstream>

#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>


using namespace std;

class stringTokenizer {
public:
  inline stringTokenizer(string &str) : str(str), position(0){};

  inline string next(string search, bool returnTail = false) {
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

  inline string tail() {
    size_t oldPosition = position;
    position = str.length();
    return str.substr(oldPosition);
  };

private:
  string str;
  size_t position;
};

typedef map<string, string> stringMap;

struct URI {

  string url = "";

  inline void parseParameters() {
    stringTokenizer qt(querystring);
    do {
      string key = qt.next("=");

      if (key == "")
        break;

      parameters[key] = qt.next("&", true);
    } while (true);

  }

  inline string getRequestURL() {
    return url;
  }

  inline URI(string input, bool shouldParseParameters = false) {

    //save the initial request to avoid redundant code
    url = input;

    stringTokenizer t = stringTokenizer(input);
    protocol = t.next("://");

    string hostPortString = t.next("/");

    stringTokenizer hostPort(hostPortString);

    host = hostPort.next(hostPortString[0] == '[' ? "]:" : ":", true);

    if (host[0] == '[')
      host = host.substr(1, host.size() - 1);

    port = hostPort.tail();

    address = t.next("?", true);
    querystring = t.next("#", true);

    hash = t.tail();

    if (shouldParseParameters)
      parseParameters();

  };

  string protocol, host, port, address, querystring, hash;
  stringMap parameters;
};


class HTTPResponse {
  public: 
    bool success;
    string protocol;
    string response;
    string responseString;

    stringMap header;

    string body;

    HTTPResponse() : success(true) {};

    static HTTPResponse error() {
      HTTPResponse result;
      result.success = false;
      return result;
    }
};

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

  static HTTPResponse request(HTTPMethod method, URI uri) {

    cout << methodTostring(method) << endl;

    cout << "URL: " << uri.getRequestURL() << '\n';
    cout << "Protocol: " << uri.protocol << '\n';
    cout << "Host : " << uri.host<< '\n';
    cout << "Port: " << uri.port << '\n';
    cout << "Address: " << uri.address << '\n';
    cout << "QueryString: " << uri.querystring << '\n';
    cout << "Hash: " << uri.hash << '\n';

    HTTPResponse hr;
    hr.success = true;

    return hr;

  }

};




