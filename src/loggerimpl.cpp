//
// Created by cory on 4/12/25.
//
#include "loggerimpl.hpp"

void Logger::info(const char *str)
{
    printf("%s\n", str);
}

void Logger::warn(const char *str)
{
    printf("%s\n", str);
}

void Logger::err(const char *str)
{
    fprintf(stderr, "%s\n", str);
}
