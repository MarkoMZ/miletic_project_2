
/*

This file contains the functionality of the client itsself.

*/

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <cstring>
#include <sstream>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
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
  inline void parseParameters() {
    stringTokenizer qt(querystring);
    do {
      string key = qt.next("=");
      if (key == "")
        break;
      parameters[key] = qt.next("&", true);
    } while (true);
  }

  inline URI(string input, bool shouldParseParameters = false) {
    stringTokenizer t = stringTokenizer(input);
    protocol = t.next("://");

    string hostPortString = t.next("/");

    stringstringTokenizer hostPort(hostPortString);

    host = hostPort.next(hostPortString[0] == '[' ? "]:" : ":", true);

    if (host[0] == '[')
      host = host.substr(1, host.size() - 1);

    port = hostPort.tail();

    address = t.next("?", true);
    querystring = t.next("#", true);

    hash = t.tail();

    if (shouldParseParameters) {
      parseParameters();
    }
  };

  string protocol, host, port, address, querystring, hash;
  stringMap parameters;
};
