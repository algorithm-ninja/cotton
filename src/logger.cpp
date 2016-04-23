#include "logger.hpp"
#include <iostream>
#include <boost/algorithm/string/replace.hpp>

void CottonTTYLogger::error(int code, const std::string& error) {
    std::cerr << error_color << "Error " << code;
    std::cerr << reset_color << ": " << error << std::endl;
}
void CottonTTYLogger::warning(int code, const std::string& warning) {
    std::cerr << warning_color << "Error " << code;
    std::cerr << reset_color << ": " << warning << std::endl;
}
void CottonTTYLogger::result(bool res) {
    std::cout << (res?"true":"false") << std::endl;
}
void CottonTTYLogger::result(size_t res) {
    std::cout << res << std::endl;
}
void CottonTTYLogger::result(const std::string& res) {
    std::cout << res << std::endl;
}
void CottonTTYLogger::result(const std::vector<std::pair<std::string, std::string>>& res) {
    for (unsigned i=0; i<res.size(); i++)
        std::cout << res[i].first << ": " << res[i].second << std::endl;
}
void CottonTTYLogger::result(const std::vector<std::tuple<std::string, int, std::vector<std::string>>>& res) {
    for (unsigned i=0; i<res.size(); i++) {
        std::cout << boxname_color << std::get<0>(res[i]) << reset_color << std::endl;
        std::cout << "penality: " << std::get<1>(res[i]) << std::endl;
        std::cout << "features: ";
        for (auto feature: std::get<2>(res[i]))
            std::cout << feature << " ";
        std::cout << std::endl;
    }
}

void CottonJSONLogger::error(int code, const std::string& error) {
    errors.emplace_back(code, error);
}
void CottonJSONLogger::warning(int code, const std::string& warning) {
    warnings.emplace_back(code, warning);
}
void CottonJSONLogger::result(bool res) {
    result_ = res?"true":"false";
}
void CottonJSONLogger::result(size_t res) {
    result_ = std::to_string(res);
}
void CottonJSONLogger::result(const std::string& res) {
    result_ = res;
    // should not be necessary
    boost::replace_all(result_, "\\", "\\\\");
    boost::replace_all(result_, "\"", "\\\"");
    result_ = '"' + result_ + '"';
}
void CottonJSONLogger::result(const std::vector<std::pair<std::string, std::string>>& res) {
    std::cout << "{";
    for (unsigned i=0; i<res.size(); i++) {
        std::cout << "\"" << res[i].first << "\": \"" << res[i].second << "\"";
        if (i+1 != res.size()) std::cout << ", ";
    }
    std::cout << "}";
}
void CottonJSONLogger::result(const std::vector<std::tuple<std::string, int, std::vector<std::string>>>& res) {
    result_ = "[";
    for (unsigned i=0; i<res.size(); i++) {
        result_ += "{\"name\": " + std::get<0>(res[i]);
        result_ += ", \"penality\": " + std::to_string(std::get<1>(res[i]));
        result_ += ", \"features\": [";
        for (unsigned j=0; j<std::get<2>(res[i]).size(); j++) {
            result_ += "\"" + std::get<2>(res[i])[j] + "\"";
            if (j+1 != std::get<2>(res[i]).size()) result_ += ", ";
        }
        result_ += "]}";
        if (i+1 != res.size()) result_ += ", ";
    }
    result_ += "]";
}
void CottonJSONLogger::write() {
    std::cout << "{\"result\": " << result_;
    std::cout << ", \"errors\": [";
    for (unsigned i=0; i<errors.size(); i++) {
        std::cout << "{\"code\": " << errors[i].first;
        std::cout << ", \"message\": \"" << errors[i].second << "\"}";
        if (i+1 != errors.size()) std::cout << ", ";
    }
    std::cout << "], \"warnings\": [";
    for (unsigned i=0; i<warnings.size(); i++) {
        std::cout << "{\"code\": " << warnings[i].first;
        std::cout << ", \"message\": \"" << warnings[i].second << "\"}";
        if (i+1 != warnings.size()) std::cout << ", ";
    }
    std::cout << "]}" << std::endl;
}
