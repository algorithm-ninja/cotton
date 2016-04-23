#ifndef COTTON_LOGGER_HPP
#define COTTON_LOGGER_HPP
#include <tuple>
#include <vector>

class CottonLogger {
public:
    virtual bool isttylogger() = 0;
    virtual void error(int code, const std::string& error) = 0;
    virtual void warning(int code, const std::string& warning) = 0;
    virtual void result(bool res) = 0;
    virtual void result(size_t res) = 0;
    virtual void result(const std::string& res) = 0;
    virtual void result(const std::vector<std::tuple<std::string, int, std::vector<std::string>>>& res) = 0;
    virtual void write() = 0;
    virtual ~CottonLogger() = default;
};

class CottonTTYLogger: public CottonLogger {
#ifdef __unix__
    static constexpr const char* const error_color = "\033[31;1m";
    static constexpr const char* const warning_color = "\033[31;3m";
    static constexpr const char* const reset_color = "\033[;m";
#else
    static constexpr const char* const error_color = "";
    static constexpr const char* const warning_color = "";
    static constexpr const char* const reset_color = "";
#endif
public:
    bool isttylogger() override {return true;};
    void error(int code, const std::string& error) override;
    void warning(int code, const std::string& warning) override;
    void result(bool res) override;
    void result(size_t res) override;
    void result(const std::string& res) override;
    void result(const std::vector<std::tuple<std::string, int, std::vector<std::string>>>& res) override;
    void write() override {};
};

class CottonJSONLogger: public CottonLogger {
    std::string result_ = "null";
    std::vector<std::pair<int, std::string>> errors;
    std::vector<std::pair<int, std::string>> warnings;
public:
    bool isttylogger() override {return false;};
    void error(int code, const std::string& error) override;
    void warning(int code, const std::string& warning) override;
    void result(bool res) override;
    void result(size_t res) override;
    void result(const std::string& res) override;
    void result(const std::vector<std::tuple<std::string, int, std::vector<std::string>>>& res) override;
    void write() override;
};

extern CottonLogger* logger;

#endif
