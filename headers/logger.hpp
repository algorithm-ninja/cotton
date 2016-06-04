#ifndef COTTON_LOGGER_HPP
#define COTTON_LOGGER_HPP
#include <tuple>
#include <vector>
#include <functional>
#include "simple_json.hpp"
#include "util.hpp"
typedef std::function<void(int, const std::string& str)> callback_t;

class CottonLogger {
    callback_t error_function;
    callback_t warning_function;
public:
    CottonLogger():
        error_function(std::bind(&CottonLogger::error, this, std::placeholders::_1, std::placeholders::_2)),
        warning_function(std::bind(&CottonLogger::warning, this, std::placeholders::_1, std::placeholders::_2))
    {}
    const callback_t& get_error_function() {return error_function;}
    const callback_t& get_warning_function() {return warning_function;}
    virtual bool isttylogger() = 0;
    virtual void error(int code, const std::string& error) = 0;
    virtual void warning(int code, const std::string& warning) = 0;
    virtual void result(bool res) = 0;
    virtual void result(int res) {result((size_t)res);}
    virtual void result(size_t res) = 0;
    virtual void result(const std::string& res) = 0;
    virtual void result(const time_limit_t& time) = 0;
    virtual void result(const space_limit_t& space) = 0;
    virtual void result(const std::vector<std::pair<std::string, std::string>>& res) = 0;
    virtual void result(const std::vector<std::tuple<std::string, int, std::vector<std::string>>>& res) = 0;
    virtual void write() = 0;
    virtual ~CottonLogger() = default;
};

class CottonTTYLogger: public CottonLogger {
    static constexpr const char* const error_color = "\033[31;1m";
    static constexpr const char* const warning_color = "\033[31;3m";
    static constexpr const char* const boxname_color = "\033[1;1m";
    static constexpr const char* const reset_color = "\033[;m";
public:
    using CottonLogger::CottonLogger;
    bool isttylogger() override {return true;};
    void error(int code, const std::string& error) override;
    void warning(int code, const std::string& warning) override;
    void result(bool res) override;
    void result(size_t res) override;
    void result(const std::string& res) override;
    void result(const time_limit_t& time) override;
    void result(const space_limit_t& space) override;
    void result(const std::vector<std::pair<std::string, std::string>>& res) override;
    void result(const std::vector<std::tuple<std::string, int, std::vector<std::string>>>& res) override;
    void write() override {};
};

class CottonJSONLogger: public CottonLogger {
    json_raw_string result_ = "null";
    std::vector<std::pair<int, std::string>> errors;
    std::vector<std::pair<int, std::string>> warnings;
public:
    using CottonLogger::CottonLogger;
    bool isttylogger() override {return false;};
    void error(int code, const std::string& error) override;
    void warning(int code, const std::string& warning) override;
    void result(bool res) override;
    void result(size_t res) override;
    void result(const std::string& res) override;
    void result(const time_limit_t& time) override;
    void result(const space_limit_t& space) override;
    void result(const std::vector<std::pair<std::string, std::string>>& res) override;
    void result(const std::vector<std::tuple<std::string, int, std::vector<std::string>>>& res) override;
    void write() override;
};

extern CottonLogger* logger;

#endif
