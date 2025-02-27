#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <string>

class Logger
{
public:
    virtual ~Logger() = default;
    virtual void log(const std::string &msg) = 0;
};

class ConsoleLogger : public Logger
{
public:
    void log(const std::string &msg) override
    {
        std::cout << "[LOG] " << msg << std::endl;
    }
};

class NullLogger : public Logger
{
public:
    void log(const std::string &) override {}
};

inline Logger &get_default_logger()
{
    static NullLogger default_logger;
    return default_logger;
}

#endif // LOGGER_HPP
