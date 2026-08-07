#include <iostream>
#include <string>
#include <chrono>
#include "json11.hpp"

int main() {
    std::string jsonStr = "{\"msg\":\"";
    for(int i=0; i<3000; i++) {
        jsonStr += "https://discord.gg/ba7ASAek7";
    }
    jsonStr += "\"}";
    
    std::cout << "JSON length: " << jsonStr.length() << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    std::string err;
    auto json = json11::Json::parse(jsonStr, err);
    auto end = std::chrono::high_resolution_clock::now();
    
    std::cout << "Parse time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << "ms" << std::endl;
    return 0;
}
