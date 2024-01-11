//
// Created by volund on 1/9/24.
//
#include "portal/config.h"
#include <boost/program_options.hpp>

namespace po = boost::program_options;
using namespace portal::config;


int main(int argc, char* argv[]) {
    try {
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("listen", po::value<std::string>(), "address and port to listen on (format: addr:port)")
            ("server", po::value<std::string>(), "address and port of the server (format: addr:port)")
            ("path", po::value<std::string>(), "relative url path to websocket. Default: /ws")
            ("secure", po::value<bool>(), "use secure websocket. Default: false");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 1;
        }

        if (vm.count("listen")) {
            std::string addr = vm["listen"].as<std::string>();
            auto pos = addr.find(':');
            if (pos != std::string::npos) {
                listenAddress = addr.substr(0, pos);
                listenPort = static_cast<uint16_t>(std::stoi(addr.substr(pos + 1)));
            }
        }

        if (vm.count("server")) {
            std::string addr = vm["server"].as<std::string>();
            auto pos = addr.find(':');
            if (pos != std::string::npos) {
                serverAddress = addr.substr(0, pos);
                serverPort = addr.substr(pos + 1);
            }
        }

        if(vm.count("path")) {
            serverPath = vm["path"].as<std::string>();
        }

        if(vm.count("secure")) {
            serverSecure = vm["secure"].as<bool>();
        }


        // Display the configuration for verification
        std::cout << "Listening on: " << listenAddress << ":" << listenPort << std::endl;
        std::cout << "Server: " << serverAddress << std::endl;

        // ... Continue with the rest of your program ...

    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}