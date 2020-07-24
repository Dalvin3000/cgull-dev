#pragma once

#include "../config.h"

#include <atomic>


CGULL_NAMESPACE_START
CGULL_GUTS_NAMESPACE_START


//! Slightly sugared version of \a atomic_int.
class ref_counted : public std::atomic<int>
{
    CGULL_DISABLE_COPY(ref_counted);

public:
    ref_counted() : std::atomic<int>(0) { }

    bool deref()    { return fetch_sub(1) - 1; }
    bool ref()      { return fetch_add(1) + 1; }

};



//! Base class for pimpl implementation data.
class shared_data
{
    CGULL_DISABLE_COPY(shared_data);

public:
    mutable ref_counted _ref;

    shared_data() {}

};



//! Smart pointer for explicitly controlled shared data.
template<typename _T>
class shared_data_ptr
{
public:
    shared_data_ptr() {}
    explicit
        shared_data_ptr(_T* from) noexcept;
    template<typename _OtherT>
    shared_data_ptr(const shared_data_ptr<_OtherT>& other);
    shared_data_ptr(const shared_data_ptr<_T>& other);
    shared_data_ptr(shared_data_ptr&& other) noexcept;
    ~shared_data_ptr();

    shared_data_ptr<_T>& operator=(const shared_data_ptr<_T>& other);
    shared_data_ptr<_T>& operator=(shared_data_ptr<_T>&& other) noexcept;
    shared_data_ptr<_T>& operator=(_T* other);

    void detach() ;
    void reset();
    void swap(shared_data_ptr &other) noexcept;

    _T*         data() const        { return d; }
    const _T*   cdata() const       { return d; }

    _T& operator*() const           { return* d; }
    _T* operator->()                { return d; }
    _T* operator->() const          { return d; }

    operator bool() const           { return d != nullptr; }

    bool operator==(const shared_data_ptr<_T>& other) const   { return d == other.d; }
    bool operator!=(const shared_data_ptr<_T>& other) const   { return d != other.d; }
    bool operator==(const _T* ptr) const                      { return d == ptr; }
    bool operator!=(const _T* ptr) const                      { return d != ptr; }

    bool operator!() const { return !d; }

protected:
    _T* clone();

private:
    _T* d = nullptr;

};


CGULL_GUTS_NAMESPACE_END
CGULL_NAMESPACE_END


CGULL_NAMESPACE_START
CGULL_GUTS_NAMESPACE_START


template<typename _T> inline
shared_data_ptr<_T>::shared_data_ptr(_T* from) noexcept
    : d(from)
{
    if(d)
        d->_ref.ref();
}

template<typename _T> inline
shared_data_ptr<_T>::shared_data_ptr(const shared_data_ptr<_T>& other)
    : d(other.d)
{
    if(d)
        d->_ref.ref();
}

template<typename _T>
template<typename _OtherT> inline
shared_data_ptr<_T>::shared_data_ptr(const shared_data_ptr<_OtherT>& other)
    : d(static_cast< _T* >(other.data()))
{
    if(d)
        d->_ref.ref();
}

template<typename _T> inline
shared_data_ptr<_T>::shared_data_ptr(shared_data_ptr&& other) noexcept
    : d(other.d)
{
    other.d = nullptr;
}

template<typename _T> inline
shared_data_ptr<_T>::~shared_data_ptr()
{
    if(d && !d->_ref.deref())
        delete d;
}

template<typename _T> inline
shared_data_ptr<_T>& shared_data_ptr<_T>::operator=(const shared_data_ptr<_T>& other)
{
    if(other.d != d)
    {
        if(other.d)
            other.d->_ref.ref();

        _T* old = d;
        d = other.d;

        if(old && !old->_ref.deref())
            delete old;
    }

    return *this;
}

template<typename _T> inline
shared_data_ptr<_T>& shared_data_ptr<_T>::operator=(shared_data_ptr<_T>&& other) noexcept
{
    std::swap(d, other.d);

    return *this;
}

template<typename _T> inline
shared_data_ptr<_T>& shared_data_ptr<_T>::operator=(_T* other)
{
    if(other != d)
    {
        if(other)
            other->_ref.ref();

        _T* old = d;
        d = other;

        if(old && !old->_ref.deref())
            delete old;
    }

    return *this;
}

template<typename _T> inline
void shared_data_ptr<_T>::detach()
{
    if(d && d->_ref.load() != 1)
    {
        _T* x = clone();
        x->_ref.ref();

        if(!d->_ref.deref())
            delete d;

        d = x;
    };
}

template<typename _T> inline
void shared_data_ptr<_T>::reset()
{
    if(d && !d->_ref.deref())
        delete d;

    d = nullptr;
}

template<typename _T> inline
void shared_data_ptr<_T>::swap(shared_data_ptr &other) noexcept
{
    std::swap(d, other.d);
}

template<typename _T> inline
_T* shared_data_ptr<_T>::clone()
{
    return new _T(*d);
}


CGULL_GUTS_NAMESPACE_END
CGULL_NAMESPACE_END
