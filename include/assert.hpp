#pragma once

#include <string>

template <unsigned int LINE> void debug_assert(bool b) {
#ifdef DEBUG
    if (!b)
        throw "Assertion failed at Line:" + std::to_string(LINE) + " !";
#endif
}
