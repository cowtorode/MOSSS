//
// Created by cory on 4/12/25.
//

#ifndef CLAMS_LOGGERIMPL_HPP
#define CLAMS_LOGGERIMPL_HPP


#include <cstdio>
#include <utility>

class Logger
{
public:
    void info(const char* str);

    template<typename... Args>
    void info(const char* str, Args... args)
    {
        printf(str, std::forward<Args>(args)...);
        printf("\n");
    }

    void warn(const char* str);

    template<typename... Args>
    void warn(const char* str, Args... args)
    {
        printf(str, std::forward<Args>(args)...);
        printf("\n");
    }

    void err(const char* str);

    template<typename... Args>
    void err(const char* str, Args... args)
    {
        fprintf(stderr, str, std::forward<Args>(args)...);
        fprintf(stderr, "\n");
    }
};


#endif //CLAMS_LOGGERIMPL_HPP
