// Copyright 2018 Vladislav Yaremenko
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

namespace CGull::guts
{

    template< typename _T > inline constexpr
    RefCounter<_T>::RefCounter(const int v) noexcept
        : std::atomic<int>(v)
    { }

    template< typename _T > inline
    bool RefCounter<_T>::refnz()
    {
        for(;;)
        {
            const auto ccount = load();

            if(!ccount)
                return false;

            auto count = ccount;

            if(compare_exchange_weak(count, ccount+1))
                return true;
            else
                count = ccount;
        };
    }


    template<typename _T> inline
    SharedPtr<_T>::SharedPtr(_T* from) noexcept
        : _d(from)
    {
        if(_d)
            _d->ref();
    }

    template<typename _T> inline
    SharedPtr<_T>::SharedPtr(const SharedPtr<_T>& other)
        : _d(other._d)
    {
        if(_d)
            _d->ref();
    }

    template<typename _T>
    template<typename _OtherT> inline
    SharedPtr<_T>::SharedPtr(const SharedPtr<_OtherT>& other)
        : _d(static_cast< _T* >(other.data()))
    {
        if(_d)
            _d->ref();
    }

    template<typename _T> inline
    SharedPtr<_T>::SharedPtr(SharedPtr&& other) noexcept
        : _d(other._d)
    {
        other._d = nullptr;
    }

    template<typename _T> inline
    SharedPtr<_T>::~SharedPtr()
    {
        if(_d && !_d->deref())
            delete _d;
    }

    template<typename _T> inline
    SharedPtr<_T>& SharedPtr<_T>::operator=(const SharedPtr<_T>& other)
    {
        if(other._d != _d)
        {
            if(other._d)
                other._d->ref();

            _T* old = _d;
            _d = other._d;

            if(old && !old->deref())
                delete old;
        }

        return *this;
    }

    template<typename _T> inline
    SharedPtr<_T>& SharedPtr<_T>::operator=(SharedPtr<_T>&& other) noexcept
    {
        std::swap(_d, other._d);

        return *this;
    }

    template<typename _T> inline
    SharedPtr<_T>& SharedPtr<_T>::operator=(_T* other)
    {
        if(other != _d)
        {
            if(other)
                other->ref();

            _T* old = _d;
            _d = other;

            if(old && !old->deref())
                delete old;
        }

        return *this;
    }

    template<typename _T> inline
    void SharedPtr<_T>::detach()
    {
        if(_d && _d->_ref.get() != 1)
        {
            _T* x = clone();
            x->ref();

            if(!_d->deref())
                delete _d;

            _d = x;
        };
    }

    template<typename _T> inline
    void SharedPtr<_T>::reset()
    {
        if(_d && !_d->deref())
            delete _d;

        _d = nullptr;
    }

    template<typename _T> inline
    void SharedPtr<_T>::swap(SharedPtr &other) noexcept
    {
        std::swap(_d, other._d);
    }

    template<typename _T> inline
    _T* SharedPtr<_T>::clone()
    {
        return new _T(*_d);
    }


    inline
    WeakPtrState::Flag::Flag()
        : RefCounter<Flag>(0)
    { }

    inline
    void WeakPtrState::Flag::invalidate()
    {
        assert("Check thread here" || (refsCount() == 1));
        _isValid = false;
    }

    inline
    bool WeakPtrState::Flag::isValid() const
    {
        assert("Check thread here");
        return _isValid;
    };


    inline
    WeakPtrState::WeakPtrState(WeakPtrState::Flag* flag)
        : _flag(flag)
    { }


    inline
    WeakPtrStateHolder::~WeakPtrStateHolder()
    {
        invalidate();
    }

    inline
    WeakPtrState WeakPtrStateHolder::state() const
    {
        if(!hasRefs())
            _flag = new WeakPtrState::Flag();

        return WeakPtrState(_flag.data());
    }

    inline
    bool WeakPtrStateHolder::hasRefs() const
    {
        return _flag.data() && _flag->refsCount() > 1;
    }

    inline
    void WeakPtrStateHolder::invalidate()
    {
        if(_flag)
        {
            _flag->invalidate();
            _flag = nullptr;
        };
    }


    inline
    WeakPtrBase::WeakPtrBase(const WeakPtrState& ref)
        : _state(ref)
    { }


    template< typename _Derived > static inline
    WeakPtr<_Derived> SupportsWeakPtrBase::staticAsWeakPtr(_Derived* t)
    {
        using convertible = std::is_convertible<_Derived, SupportsWeakPtrBase&>;
        static_assert(convertible::value, "Check that AsWeakPtr argument inherits from SupportsWeakPtr");

        return _staticAsWeakPtr<_Derived>(t, *t);
    }

    template<
        typename _Derived,
        typename _Base
    > static inline
    WeakPtr<_Derived> SupportsWeakPtrBase::_staticAsWeakPtr(_Derived* t, const SupportsWeakPtr<_Base>&)
    {
        WeakPtr<_Base> ptr = t->_Base::createWeakPtr();

        return WeakPtr<_Derived>{ptr._state, static_cast<_Derived*>(ptr._d)};
    }


    template<typename _T>
    template<typename _O> inline
    WeakPtr<_T>::WeakPtr(const WeakPtr<_O>& other)
        : WeakPtrBase(other)
        , _d(other.data())
    { }

    template<typename _T> inline
    WeakPtr<_T>::WeakPtr(const WeakPtrState& state, _T* from)
        : WeakPtrBase(state)
        , _d(from)
    { }

    template<typename _T> inline
    _T& WeakPtr<_T>::operator*() const
    {
        assert(data() != nullptr);
        return *data();
    }

    template<typename _T> inline
    _T* WeakPtr<_T>::operator->() const
    {
        assert(data() != nullptr);
        return data();
    }

    template<typename _T> inline
    void WeakPtr<_T>::reset()
    {
        _state = WeakPtrState();
        _d = nullptr;
    }


    template <class _T> inline
    WeakPtr<_T> SupportsWeakPtr<_T>::createWeakPtr() const
    {
        return WeakPtr<_T>{_stateHolder.state(), (_T*)(this)};
    }


    template <class _T> inline
    WeakPtrFactory<_T>::WeakPtrFactory(_T* from)
        : _d(from)
    { }

    template <class _T> inline
    WeakPtrFactory<_T>::~WeakPtrFactory()
    {
        _d = nullptr;
    }

    template <class _T> inline
    WeakPtr<_T> WeakPtrFactory<_T>::createWeakPtr()
    {
        assert(_d);
        return WeakPtr<_T>{_stateHolder.state(), _d};
    }

    template <class _T> inline
    void WeakPtrFactory<_T>::invalidateWeakPtrs()
    {
        assert(_d);
        _stateHolder.invalidate();
    }

    template <class _T> inline
    bool WeakPtrFactory<_T>::hasWeakPtrs() const
    {
        assert(_d);
        return _stateHolder.hasRefs();
    }

}
