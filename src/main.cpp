/*

    author:     MILETIC Marko
    class:      5CHIF
    description:
        The goal of this project is to implement an simple HTTP 1.1 Client that is able to download
        and upload files to a server.

        HTTP-Basic Authentication should also be implemented.

        This programm should be used via CLI.

*/

#include <iostream>
#include <map>
#include <fstream>
#include <chrono>

#include "spdlog/spdlog.h"
#include "CLI11.hpp"

#include "../include/http_client.hpp"
#include "HTTPResponseObject.pb.h"
#include <json.hpp>
#include <rang.hpp>


using namespace std;
using json = nlohmann::json;

int main(int argc, char* argv[]) {
    
    chrono::steady_clock::time_point begin = chrono::steady_clock::now();

    CLI::App app{"HTTPipe ~ MILETIC Marko"};

    // Create HTTPResponseObject.
    HTTPResponseObject::HTTPResponseObject hro;
    hro.set_success(true);

    spdlog::get("http_client_logger")
        ->set_pattern("[HTTPipe] [%l] %v");

    auto console_out = spdlog::stdout_color_mt("console_out");	
    auto console_sink = dynamic_cast<spdlog::sinks::stdout_color_sink_mt *>(console_out->sinks().back().get());
    console_sink->set_color_mode(spdlog::color_mode::always);

    console_sink->set_color(spdlog::level::info, "\033[36m");
    console_sink->set_color(spdlog::level::err, "\033[31m"); 
    console_sink->set_color(spdlog::level::critical, "\033[33m");  


    spdlog::get("http_client_logger")
        ->info("MILETIC HTTPipe");


    // Define options.
    string method;
    string file_path;
    string url_str;
    string auth_data;
    string save_dir;
    string json_file_name;
    string cookie;

    app.add_option("-m, --method", method, "HTTP-Method")
        ->required();

    app.add_option("-f,--file", file_path, 
                   "Path to file (Send via POST / PUT)")
        ->check(CLI::Validator(CLI::ExistingFile));

    app.add_option("-u, --URL, -l, --link", url_str, 
                   "URL to ressource")
        ->required();

    app.add_option("-a, --auth", auth_data, 
                   "Authentication data -> [Username:Password]");

    app.add_option("-c, --cookie", cookie, 
                   "Cookie -> [Name=Value]");

    app.add_option("-d, --dir", save_dir, 
                   "Directory in which downloaded files should be saved");

    app.add_option("-j, --json", json_file_name, 
                   "Authentication data in json format")
        ->check(CLI::Validator(CLI::ExistingFile));;

    // Parse the passed CLI-Options.
    CLI11_PARSE(app, argc, argv);

    // If a JSON was passed, pipe the content into a JSON object.
    json json_data;
    if(json_file_name.compare("") != 0) {
        ifstream i(json_file_name);
        
        i >> json_data;
    }

    // Run the request and save the return value.
    spdlog::get("http_client_logger")->info("Starting request...");

    HTTPResponseObject::HTTPResponseObject response = 
    HTTPClient::request(HTTPClient::stringToMethod(method), 
                        URL(url_str), 
                        json_data, file_path, 
                        auth_data, save_dir, cookie);
    
    // Check if the request was successfully processed.
    if(!(bool)response.success()) {
        spdlog::get("http_client_logger")
            ->critical("The given request was not processed!");
        return -1;
    } else {
        chrono::steady_clock::time_point end = chrono::steady_clock::now();
        spdlog::get("http_client_logger")
            ->info("The given request was successfully processed!");

        auto seconds = chrono::duration_cast<chrono::microseconds>(end - begin).count() / 1000000.0;
        
        cout << '\n';

        spdlog::get("http_client_logger")
            ->info("Elapsed time since request: {} seconds", seconds);

        return 0;
    }
}
