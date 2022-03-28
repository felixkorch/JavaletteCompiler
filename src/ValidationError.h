#pragma once
#include <iostream>
#include <exception>
#include <cstring>

class ValidationError : public std::exception {
    std::string msg_;
public:
    ValidationError(const std::string& msg)
    {
        msg_ = "Javalette validation error: \"" + msg + "\"";
    }

	const char * what () const throw ()
    {
    	return msg_.c_str();
    }
};

class TypeError : public std::exception {
    std::string msg_;
public:
    TypeError(const std::string& msg)
    {
        msg_ = "Javalette Type error: " + msg;
    }

    const char * what () const throw ()
    {
        return msg_.c_str();
    }
};