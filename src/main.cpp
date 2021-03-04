/*

    author:     MILETIC Marko
    class:      5CHIF
    description:
        The goal of this project is to implement an simple HTTP 1.1 Client that is able to download
        and upload files to a server.

        HTTP-Basic Authentication should also be implemented.

        This programm should be used via CLI.

        But out of own interest i want to later add an GUI to get a better user expirience.

*/

#include <iostream>
#include <map>
#include <thread>

#include "../include/http_client.hpp"

using namespace std;

int main(int argc, char* argv[]) {
    if(argc == 1) 
        // error
        return 1;

    //Testing
    HTTPResponse response = HTTPClient::request(HTTPClient::GET, URI(argv[1]));
  
    if(!response.success) {
        cout << "There was an error while processing your request!" << endl;
        return -1;
    } else {
        cout << "Success!" << endl;
        return 0;
    }
}
