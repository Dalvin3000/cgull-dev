#include "promise.h"

namespace CGull::guts
{
    std::atomic_int& _debugPrivateCounter()
    {
        static std::atomic_int v = 0;

        return v;
    };
};
