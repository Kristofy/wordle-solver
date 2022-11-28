#ifndef TIMER_HPP
#define TIMER_HPP

#include <chrono>
#include <string>
#include <iostream>

#define S_(x) #x
#define CONCAT_(x, y) x##y
#define CONCAT(x, y) CONCAT_(x, y)
#define TimeFunc() Timer CONCAT(t_, __COUNTER__)(__PRETTY_FUNCTION__);
#define TimeScope(name) Timer CONCAT(t_, __LINE__)(name);

struct Timer
{

    std::string name;
    std::chrono::time_point<std::chrono::steady_clock> start;
    bool ended;

    Timer(std::string &&name)
        : name(std::move(name)), start(std::chrono::steady_clock::now()), ended(false)
    {}

    ~Timer()
    {
        if (!ended)
        {
            double d = end();
            std::cout << name << ": " << d << "ms" << std::endl;
        }
    }

    double end()
    {
        auto endOfTime = std::chrono::steady_clock::now();
        auto duration = endOfTime - start;
        ended = true;
        return std::chrono::duration<double, std::milli>(duration).count();
    }
};

#endif