#pragma once
#include <cstring>
#include <exception>
#include <iostream>
#include <sstream>

namespace jlc::typechecker {

class TypeError : public std::exception {
    std::string msg_;

  public:
    TypeError(const std::string& msg, int lineNr, int charNr) {
        std::stringstream ss;
        ss << "ERROR:" << lineNr << ":" << charNr << ": " << msg << std::endl;
        msg_ = ss.str();
    }

    explicit TypeError(const std::string& msg) {
        std::stringstream ss;
        ss << "ERROR: " << msg << std::endl;
        msg_ = ss.str();
    }

    const char* what() const throw() { return msg_.c_str(); }
};

} // namespace jlc::typechecker