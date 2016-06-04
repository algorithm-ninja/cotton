#include "logger.hpp"
#include <iostream>

void CottonTTYLogger::error(int code, const std::string& error) {
    std::cerr << error_color << "Error " << code;
    std::cerr << reset_color << ": " << error << std::endl;
}
void CottonTTYLogger::warning(int code, const std::string& warning) {
    std::cerr << warning_color << "Warning " << code;
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
void CottonTTYLogger::result(const time_limit_t& time) {
    result(time.to_string());
}
void CottonTTYLogger::result(const space_limit_t& space) {
    result(space.to_string());
};
void CottonTTYLogger::result(const std::vector<std::pair<std::string, std::string>>& res) {
    for (unsigned i=0; i<res.size(); i++)
        std::cout << res[i].first << ": " << res[i].second << std::endl;
}
void CottonTTYLogger::result(const std::vector<std::tuple<std::string, int, std::vector<std::string>>>& res) {
    for (unsigned i=0; i<res.size(); i++) {
        std::cout << boxname_color << std::get<0>(res[i]) << reset_color << std::endl;
        std::cout << "overhead: " << std::get<1>(res[i]) << std::endl;
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
    result_ = to_json(res);
}
void CottonJSONLogger::result(size_t res) {
    result_ = to_json(res);
}
void CottonJSONLogger::result(const std::string& res) {
    result_ = to_json(res);
}
void CottonJSONLogger::result(const std::vector<std::pair<std::string, std::string>>& res) {
    result_ = to_json_obj(res);
}
void CottonJSONLogger::result(const time_limit_t& time) {
    result_ = to_json(time.double_seconds());
}
void CottonJSONLogger::result(const space_limit_t& space) {
    result_ = to_json(space.kilobytes());
};
void CottonJSONLogger::result(const std::vector<std::tuple<std::string, int, std::vector<std::string>>>& res) {
    result_ = to_json_arr(res, [](const std::tuple<std::string, int, std::vector<std::string>>& v) {
        return to_json_obj(
            "name", std::get<0>(v),
            "overhead", std::get<1>(v),
            "features", std::get<2>(v)
        );
    });
}
void CottonJSONLogger::write() {
    auto ew_converter = [] (const std::pair<int, std::string>& msg) {
        return to_json_obj("code", msg.first, "message", msg.second);
    };
    std::cout << to_json_obj(
        "result", result_,
        "errors", to_json_arr(errors, ew_converter),
        "warnings", to_json_arr(warnings, ew_converter)
    ) << std::endl;
}
