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
void CottonTTYLogger::result(const std::vector<std::tuple<std::string, int, std::vector<std::string>>>& res) {
    //std::cout << res << std::endl;
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
void CottonJSONLogger::result(const std::vector<std::tuple<std::string, int, std::vector<std::string>>>& res) {
    //std::cout << res << std::endl;
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
