
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

int main() {
    // stdout_logger_st, stderr_logger_mt, stderr_logger_st  
    auto console = spdlog::stderr_color_mt("console");

    console->set_level(spdlog::level::trace);

    console->trace("Welcome to spdlog!");
    console->debug("Welcome to spdlog!");
    console->info("Welcome to spdlog!");
    console->warn("Welcome to spdlog!");
    console->error("Welcome to spdlog!");  // Achtung: spdlog::level::err
    console->critical("Welcome to spdlog!");
    // + spdlog::level::off
    
    console->info("Welcome {}!", "Maxi");
}
