#pragma once
#include <string>
#include <stdexcept>

struct ServiceError {
    int http_status;
    std::string message;
};

class ServiceException : public std::exception {
public:
    ServiceException(ServiceError error) : error_(error) {}
    
    const char* what() const noexcept override {
        return error_.message.c_str();
    }

    const ServiceError& error() const {
        return error_;
    }

private:
    ServiceError error_;
};
