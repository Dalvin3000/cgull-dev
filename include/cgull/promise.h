#pragma once

#include "config.h"
#include "promise_private.h"
#include "handler.h"


CGULL_NAMESPACE_START


class promise
{
public:

public:
    using private_type = typename promise_private::type;


    promise()
        : _d(new promise_private{})
    { }

    const std::any& value() const { return _d->result; }


    promise& resolve(const std::any& value);
    promise& resolve(std::any&& value);
    promise& resolve();

    promise& reject(const std::any& value);
    promise& reject(std::any&& value);
    promise& reject();

    fulfillment_state_t   fulfillment() const
    {
        return _d->fulfillment_state;
    }

    bool    is_resolved() const { return fulfillment() == resolved; }
    bool    is_rejected() const { return fulfillment() == rejected; }


#if defined(CGULL_DEBUG_GUTS)
    private_type _private() const   { return _d; }
#endif

private:
    private_type _d;

    void _fulfill(std::any&& value, bool is_resolve);
    void _fulfill(const std::any& value, bool is_resolve);

};


CGULL_NAMESPACE_END


CGULL_NAMESPACE_START


inline
promise& promise::resolve(const std::any& value)
{
    _fulfill(value, true);

    return *this;
}


inline
promise& promise::resolve(std::any&& value)
{
    _fulfill(value, true);

    return *this;
}


inline
promise& promise::resolve()
{
    _fulfill({}, true);

    return *this;
}


inline
promise& promise::reject(const std::any& value)
{
    _fulfill(value, false);

    return *this;
}


inline
promise& promise::reject(std::any&& value)
{
    _fulfill(value, false);

    return *this;
}


inline
promise& promise::reject()
{
    _fulfill({}, false);

    return *this;
}


inline
void promise::_fulfill(std::any&& value, bool is_resolve)
{
    _cgull_d;

    _cgull_context_local
        ->local_fulfill(std::forward<decltype(value)>(value), is_resolve ? resolved : rejected);
    _cgull_async
        ->fulfill(std::forward<decltype(value)>(value), is_resolve);
}


inline
void promise::_fulfill(const std::any& value, bool is_resolve)
{
    _cgull_d;

    // Copy value here cause we will use it in other ctx or copy it to save in promise a/w.
    _cgull_context_local
        ->local_fulfill(std::any{value}, is_resolve ? resolved : rejected);
    _cgull_async
        ->fulfill(std::any{value}, is_resolve);
}



CGULL_NAMESPACE_END
