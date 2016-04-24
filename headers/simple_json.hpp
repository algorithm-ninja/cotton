#ifndef COTTON_SIMPLE_JSON_HPP
#define COTTON_SIMPLE_JSON_HPP
#include <string>
#include <vector>

class json_raw_string: public std::string {
public:
    json_raw_string(const std::string& s): std::string(s) {}
    json_raw_string(const char* s): std::string(s) {}
};

template<typename T>
json_raw_string to_json(const T& val) {
    return std::to_string(val);
}

template<>
json_raw_string to_json(const bool& val);

template<>
json_raw_string to_json(const std::string& val);

template<>
json_raw_string to_json(const json_raw_string& val);

json_raw_string to_json_obj_rec();

template<typename T, typename U>
json_raw_string to_json_arr(const std::vector<T>& val, const U& conv_fun) {
    std::string res = "[";
    for (unsigned i=0; i<val.size(); i++) {
        res += conv_fun(val[i]);
        if (i+1 != val.size()) res += ", ";
    }
    return res + "]";
}

template<typename T>
json_raw_string to_json(const std::vector<T>& val) {
    return to_json_arr(val, [](const T& val) {return to_json(val);});
}

template<typename T, typename... Args>
json_raw_string to_json_obj_rec(const std::string& key, const T& val, Args... args) {
    std::string ret = to_json(key) + ": " + to_json(val);
    if (sizeof...(args))
        return ret + ", " + to_json_obj_rec(args...);
    return ret + "}";
}

template<typename T, typename... Args>
json_raw_string to_json_obj(const std::string& key, const T& val, Args... args) {
    return "{" + to_json_obj_rec(key, val, args...);
}

template<typename T>
json_raw_string to_json_obj(const std::vector<std::pair<std::string, T>>& obj) {
    std::string res = "{";
    for (unsigned i=0; i<obj.size(); i++) {
        res += to_json(obj[i].first) + ": " + to_json(obj[i].second);
        if (i+1 != res.size()) res += ", ";
    }
    return res + "}";
}
#endif
