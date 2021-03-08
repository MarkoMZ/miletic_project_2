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

#include "spdlog/spdlog.h"
#include "CLI11.hpp"

#include "../include/http_client.hpp"


using namespace std;

int main(int argc, char* argv[]) {
    if(argc == 1) 
        // Too few Arguments
        return 1;

    spdlog::get("http_client_logger")-> set_pattern("[HTTP 1.1 Client] [%l] %v");

    
    spdlog::get("http_client_logger")->info("MILETIC HTTP-Client 1.1");

    spdlog::get("http_client_logger")->info("Starting request...");
    HTTPResponse response = HTTPClient::request(HTTPClient::GET, URI(argv[1]));
    
  
    if(!response.success) {
        spdlog::get("http_client_logger")->critical("The given request was not processed!");
        return -1;
    } else {
        spdlog::get("http_client_logger")->info("The given request was successfully processed!");
        return 0;
    }
}
