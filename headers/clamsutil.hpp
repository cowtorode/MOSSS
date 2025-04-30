//
// Created by cory on 1/16/25.
//

#ifndef CLAMS_CLAMSUTIL_HPP
#define CLAMS_CLAMSUTIL_HPP

#define DEBUG

#ifdef DEBUG

#include <iostream>
#define debug(stmt) stmt

#elif

// do nothing
#define debug(stmt)

#endif

#endif //CLAMS_CLAMSUTIL_HPP
