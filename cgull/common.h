// Copyright 2018 Vladislav Yaremenko
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

#pragma once

#include <any>
#include <atomic>
#include <vector>
#include <functional>
#include <typeinfo>
#include <type_traits>
#include <memory>

#define QGULL_FULL_ASYNC 1

#if 0
#   define CGULL_DEBUG
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#   define CGULL_VISIBILITY_HELPER_DLL_IMPORT __declspec(dllimport)
#   define CGULL_VISIBILITY_HELPER_DLL_EXPORT __declspec(dllexport)
#   define CGULL_VISIBILITY_HELPER_DLL_LOCAL
#else
#   if __GNUC__ >= 4
#       define CGULL_VISIBILITY_HELPER_DLL_IMPORT __attribute__ ((visibility ("default")))
#       define CGULL_VISIBILITY_HELPER_DLL_EXPORT __attribute__ ((visibility ("default")))
#       define CGULL_VISIBILITY_HELPER_DLL_LOCAL  __attribute__ ((visibility ("hidden")))
#   else
#       define CGULL_VISIBILITY_HELPER_DLL_IMPORT
#       define CGULL_VISIBILITY_HELPER_DLL_EXPORT
#       define CGULL_VISIBILITY_HELPER_DLL_LOCAL
#   endif
#endif

#if defined(CGULL_SHARED)
#   if defined(CGULL_EXPORTS)
#       define CGULL_API    CGULL_VISIBILITY_HELPER_DLL_EXPORT
#       define CGULL_EXTERN
#   else
#       define CGULL_API    CGULL_VISIBILITY_HELPER_DLL_IMPORT
#       define CGULL_EXTERN extern
#   endif
#   define CGULL_LOCAL CGULL_VISIBILITY_HELPER_DLL_LOCAL
#else
#   define CGULL_API
#   define CGULL_LOCAL
#   define CGULL_EXTERN
#endif

#define CGULL_DISABLE_COPY(c)           \
    c(const c&) = delete;               \
    c& operator=(const c&) = delete;

//! \todo this allocator is not dyn inheritance friendly
//! \todo thread_local is also not a panacea. need to be replaced
#define CGULL_DECLARE_ALLOCATOR(c)                  \
    struct /*CGULL_API*/ Allocator##c##Wrapper          \
    {                                               \
        using Allocator##c = std::allocator<c>;     \
                                                    \
        static Allocator##c##Wrapper& instance();   \
        c* allocate(size_t count);                  \
        void deallocate(c* ptr, size_t count);      \
                                                    \
        Allocator##c allocator;                     \
    };

#define CGULL_DEFINE_ALLOCATOR(c)                                   \
                                                                    \
    inline                                                          \
    Allocator##c##Wrapper& Allocator##c##Wrapper::instance()        \
    {                                                               \
        thread_local static Allocator##c##Wrapper r;                \
                                                                    \
        return r;                                                   \
    }                                                               \
                                                                    \
    inline                                                          \
    c* Allocator##c##Wrapper::allocate(size_t count)                \
    {                                                               \
        return allocator.allocate(count);                           \
    }                                                               \
                                                                    \
    inline                                                          \
    void Allocator##c##Wrapper::deallocate(c* ptr, size_t count)    \
    {                                                               \
        allocator.deallocate(ptr, count);                           \
    }                                                               \



#define CGULL_USE_ALLOCATOR(c)                      \
    public:                                         \
        static inline                               \
        void* operator new(std::size_t sz)          \
        {                                           \
            return Allocator##c##Wrapper            \
                ::instance().allocate(1);           \
        }                                           \
        static inline                               \
        void  operator delete(void* p)              \
        {                                           \
            Allocator##c##Wrapper                   \
                ::instance().deallocate((c*)p, 1);  \
        }




namespace CGull
{
    //! Represents promise finish finishState.
    enum FinishState
    {
        NotFinished = 0,
        AwaitingResolve,
        AwaitingReject,
        Thenned,
        Rescued,
        Skipped,
    };

    using AtomicFinishState = std::atomic<FinishState>;

    //! Represents promise fulfillment finishState.
    enum FulfillmentState
    {
        NotFulfilled = 0,
        FulfillingNow,
        Resolved,
        Rejected,
        Aborted,
    };

    using AtomicFulfillmentState = std::atomic<FulfillmentState>;

    //! Represents promise wait type for complex nested promises.
    enum WaitType
    {
        Any = 0,
        All,
        First,
        LastBound,
    };

    using AtomicWaitType = std::atomic<WaitType>;

    //! Lamda return sugar.
    static constexpr bool Abort = true;
    //! Lamda return sugar.
    static constexpr bool Execute = false;

    using AnyList = std::vector<std::any>;

    //! Operation callback handler type.
    //! \param abort used for resource release on functor deletion.
    using Callback = void(bool abort, std::any&& innersResult);
    using CallbackFunctor = std::function<Callback>;

};




namespace CGull::guts
{
#if defined(CGULL_DEBUG_GUTS)
    CGULL_API std::atomic_int& _debugPrivateCounter();
#endif


    //! \return [value] true if type \a _T is one of \a _OtherT types.
    template<typename _T, typename ... _OtherT>
    struct one_of_types : std::disjunction< std::is_same<_OtherT, _T> ... >::type {};



    //! Slightly sugared version of \a atomic_int.
    class RefCounter : public std::atomic<int>
    {
        RefCounter(const RefCounter&) = delete;
        RefCounter& operator=(const RefCounter&) = delete;

    public:
        RefCounter()   : std::atomic<int>(0) { }

        bool deref()    { return fetch_sub(1) - 1; }
        bool ref()      { return fetch_add(1) + 1; }

    };



    //! Base class for pimpl implementation data.
    class SharedData
    {
        SharedData& operator=(const SharedData&) = delete;

    public:
        mutable RefCounter _ref;

        SharedData() {}
        SharedData(const SharedData&) {}

    };



    //! Smart pointer for explicitly controlled shared data.
    template<typename _T>
    class SharedDataPtr
    {
    public:
        SharedDataPtr() {}
        explicit
        SharedDataPtr(_T* from) noexcept;
        template<typename _OtherT>
        SharedDataPtr(const SharedDataPtr<_OtherT>& other);
        SharedDataPtr(const SharedDataPtr<_T>& other);
        SharedDataPtr(SharedDataPtr&& other) noexcept;
        ~SharedDataPtr();

        SharedDataPtr<_T>& operator=(const SharedDataPtr<_T>& other);
        SharedDataPtr<_T>& operator=(SharedDataPtr<_T>&& other) noexcept;
        SharedDataPtr<_T>& operator=(_T* other);

        void detach() ;
        void reset();
        void swap(SharedDataPtr &other) noexcept;

        _T*         data() const        { return d; }
        const _T*   constData() const   { return d; }

        _T& operator*() const           { return* d; }
        _T* operator->()                { return d; }
        _T* operator->() const          { return d; }

        operator bool() const           { return d != nullptr; }

        bool operator==(const SharedDataPtr<_T>& other) const   { return d == other.d; }
        bool operator!=(const SharedDataPtr<_T>& other) const   { return d != other.d; }
        bool operator==(const _T* ptr) const                      { return d == ptr; }
        bool operator!=(const _T* ptr) const                      { return d != ptr; }

        bool operator!() const { return !d; }

    protected:
        _T* clone();

    private:
        _T* d = nullptr;

    };

};


//CGULL_EXTERN template struct CGULL_API std::atomic<CGull::FinishState>;
//CGULL_EXTERN template struct CGULL_API std::atomic<CGull::WaitType>;
//CGULL_EXTERN template class CGULL_API std::function<CGull::guts::Callback>;
//CGULL_EXTERN template class CGULL_API std::function<CGull::guts::Callback2>;

#include "common.hpp"
