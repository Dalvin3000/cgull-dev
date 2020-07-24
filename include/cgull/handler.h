#pragma once

#include "config.h"

#include <any>


CGULL_NAMESPACE_START

class handler
{
    handler() = default;

    virtual ~handler() = 0;

public:

    virtual void fulfill(std::any&& value, bool resolve) = 0;
    virtual void try_finish() = 0;


};

CGULL_NAMESPACE_END
