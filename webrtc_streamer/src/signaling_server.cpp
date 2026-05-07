#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <mutex>

#include "rtc/rtc.hpp"

int main() {
    rtc::InitLogger(rtc::LogLevel::Info);

    struct ClientInfo {
        std::string role = "unknown";
    };
    std::map<std::shared_ptr<rtc::WebSocket>, ClientInfo> clients;
    std::mutex clientsMutex;

    rtc::WebSocketServer::Configuration config;
    config.port = 8889;

    auto server = std::make_shared<rtc::WebSocketServer>(config);

    server->onClient([&](std::shared_ptr<rtc::WebSocket> ws) {
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients[ws] = ClientInfo();
            std::cout << "New connection. Total: " << clients.size() << std::endl;
        }

        ws->onMessage([&, ws](rtc::message_variant message) {
            if (std::holds_alternative<std::string>(message)) {
                std::string msg = std::get<std::string>(message);
                
                // Identify role if not set
                {
                    std::lock_guard<std::mutex> lock(clientsMutex);
                    if (clients[ws].role == "unknown") {
                        if (msg == "request") clients[ws].role = "browser";
                        else if (msg.find("v=0") != std::string::npos) clients[ws].role = "streamer";
                        if (clients[ws].role != "unknown") {
                            std::cout << "Client identified as: " << clients[ws].role << std::endl;
                        }
                    }
                }

                size_t relayedCount = 0;
                {
                    std::lock_guard<std::mutex> lock(clientsMutex);
                    for (auto const& [client, info] : clients) {
                        if (client != ws) {
                            try {
                                client->send(msg);
                                relayedCount++;
                            } catch (...) {}
                        }
                    }
                }
                if (msg.length() > 0) {
                    std::cout << "Relayed " << msg.substr(0, 10) << "... to " << relayedCount << " clients" << std::endl;
                }
            }
        });

        ws->onClosed([&, ws]() {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.erase(ws);
            std::cout << "Client disconnected. Remaining: " << clients.size() << std::endl;
        });

        ws->onError([&, ws](std::string error) {
            std::cout << "Client error: " << error << std::endl;
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.erase(ws);
        });
    });

    std::cout << "Signaling server listening on port " << config.port << std::endl;
    std::cout << "Press Enter to stop the server..." << std::endl;
    std::cin.get();

    return 0;
}
