// Copyright 2018 Vladislav Yaremenko
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

#pragma once

namespace CGull::guts
{

    template< typename _T >
    class SharedPtr;
    template< typename _T >
    class WeakPtr;
    template< typename _T >
    class SharedPtr;
    template< typename _T >
    class SupportsWeakPtr;
    template< typename _T >
    class WeakPtrFactory;



    //! Slightly sugared version of \a atomic int.
    template< typename _T >
    class CGULL_NOVTABLE RefCounter : private std::atomic<int>
    {
        CGULL_DISABLE_COPY(RefCounter);
        friend class SharedPtr<_T>;

    public:
        constexpr
        RefCounter(const int v = 0) noexcept;

        [[nodiscard]]
        bool refnz();
        [[nodiscard]]
        bool deref()    { return fetch_sub(1) - 1; }
        void ref()      { fetch_add(1); }

        int  refsCount() const  { return load(); }
        operator int() const    { return load(); }

    };


    template<typename _T>
    class SharedPtr
    {
    public:
        SharedPtr() = default;
        explicit
        SharedPtr(_T* from) noexcept;
        template<typename _OtherT>
        SharedPtr(const SharedPtr<_OtherT>& other);
        SharedPtr(const SharedPtr<_T>& other);
        SharedPtr(SharedPtr&& other) noexcept;
        ~SharedPtr();

        SharedPtr<_T>& operator=(const SharedPtr<_T>& other);
        SharedPtr<_T>& operator=(SharedPtr<_T>&& other) noexcept;
        SharedPtr<_T>& operator=(_T* other);

        void detach();
        void reset();
        void swap(SharedPtr &other) noexcept;

        _T*         data() const      { return _d; }
        const _T*   constData() const { return _d; }

        _T& operator*() const   { return*_d; }
        _T* operator->()        { return _d; }
        _T* operator->() const  { return _d; }

        operator bool() const   { return _d != nullptr; }
        bool operator!() const  { return !_d; }

        bool operator==(const SharedPtr<_T>& other) const { return _d == other.d; }
        bool operator!=(const SharedPtr<_T>& other) const { return _d != other.d; }
        bool operator==(const _T* otherPtr) const         { return _d == otherPtr; }
        bool operator!=(const _T* otherPtr) const         { return _d != otherPtr; }

    protected:
        _T* clone();

    private:
        _T* _d = nullptr;

    };


    class WeakPtrState
    {
    public:
        class CGULL_NOVTABLE Flag : public RefCounter<Flag>
        {
            friend class RefCounter<Flag>;

        public:
            Flag();

            void invalidate();
            bool isValid() const;

        private:
            bool _isValid = true;

        };

        ~WeakPtrState() = default;
        WeakPtrState() = default;
        explicit
        WeakPtrState(Flag* flag);

        bool isValid() const  { return _flag && _flag->isValid(); };

    private:
        SharedPtr< Flag > _flag;

    };


    class  WeakPtrStateHolder
    {
    public:
        WeakPtrStateHolder() = default;
        ~WeakPtrStateHolder();

        WeakPtrState state() const;
        bool hasRefs() const;
        void invalidate();

    private:
        mutable SharedPtr< WeakPtrState::Flag > _flag;

    };


    class  WeakPtrBase
    {
    public:
        WeakPtrBase() = default;
        ~WeakPtrBase() = default;

    protected:
        WeakPtrState _state;

        explicit
        WeakPtrBase(const WeakPtrState& ref);

    };


    class SupportsWeakPtrBase
    {
    public:
        template< typename _Derived > static
        WeakPtr<_Derived> staticAsWeakPtr(_Derived* t);

    private:
        template<
            typename _Derived,
            typename _Base
        > static
        WeakPtr<_Derived> _staticAsWeakPtr(_Derived* t, const SupportsWeakPtr<_Base>&);
    };


    template< typename _T >
    class WeakPtr : public WeakPtrBase
    {
        friend class SupportsWeakPtrBase;
        friend class SupportsWeakPtr<_T>;
        friend class WeakPtrFactory<_T>;

    public:
        WeakPtr() = default;

        template< typename _O >
        WeakPtr(const WeakPtr<_O>& other);

        _T* data() const     { return _state.isValid() ? _d : nullptr; }
        operator _T*() const { return data(); }

        _T& operator*() const;
        _T* operator->() const;

        void reset();

    private:
        _T* _d = nullptr;

        WeakPtr(const WeakPtrState& state, _T* from);

    };


    template <class _T>
    class SupportsWeakPtr : public SupportsWeakPtrBase
    {
        CGULL_DISABLE_COPY(SupportsWeakPtr);

    public:
        SupportsWeakPtr() = default;

        WeakPtr<_T> createWeakPtr() const;

    protected:
        ~SupportsWeakPtr() = default;

    private:
        WeakPtrStateHolder _stateHolder;

    };


    template <class _T>
    class WeakPtrFactory
    {
        CGULL_DISABLE_COPY(WeakPtrFactory);
        WeakPtrFactory();

    public:
        explicit WeakPtrFactory(_T* from);
        ~WeakPtrFactory();

        WeakPtr<_T> createWeakPtr();
        void invalidateWeakPtrs();
        bool hasWeakPtrs() const;

    private:
        WeakPtrStateHolder _stateHolder;
        _T* _d;

    };

}

#include "memory.hpp"
