#include "simple_json.hpp"
#include <boost/algorithm/string/replace.hpp>

template<>
json_raw_string to_json(const bool& val) {
    return val?"true":"false";
}

template<>
json_raw_string to_json(const std::string& val) {
    std::string res = val;
    boost::replace_all(res, "\\", "\\\\");
    boost::replace_all(res, "\"", "\\\"");
    res = '"' + res + '"';
    return res;
}

template<>
json_raw_string to_json(const json_raw_string& val) {
    return val;
}

json_raw_string to_json_obj_rec() {
    return "";
}
