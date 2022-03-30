#pragma once
#include <iostream>
#include <exception>
#include <cstring>
#include <sstream>

class TypeError : public std::exception {
    std::string msg_;
public:

    TypeError(const std::string& msg, int lineNr, int charNr)
    {
        std::stringstream ss;
        ss << "error: " << lineNr << ":" << charNr << ": " << msg << std::endl;
        msg_ = ss.str();
    }

    explicit TypeError(const std::string& msg)
    {
        std::stringstream ss;
        ss << "error: " << msg << std::endl;
        msg_ = ss.str();
    }

    const char * what () const throw ()
    {
        return msg_.c_str();
    }
};