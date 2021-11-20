#pragma once

void debug_assert(bool b) {
#ifdef DEBUG
    if (!b) throw "Assertion failed !";
#endif
}
