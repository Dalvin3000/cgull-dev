// Copyright 2018 Vladislav Yaremenko
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

#pragma once

#include "function_traits.h"
#include "promise_private.h"
#include "sync_handler.h"

#if defined(CGULL_USE_NAMESPACE)
namespace CGull
{
#endif


//! \note All public functions int this class are thread-safe.
class /*CGULL_API*/ Promise
{
    using PrivateType = CGull::guts::PromisePrivate::Type;

    //! Constructs promise from raw private data.
    Promise(const PrivateType& from);


public:
    Promise();
    ~Promise();

    template<
        typename _Resolve,
        typename _ResolveFunctor = CGull::guts::functor_t<_Resolve>
    >
    Promise then(_Resolve&& onResolve);

    Promise& resolve(const std::any& value);
    Promise& resolve(std::any&& value);

    Promise& reject(const std::any& value);
    Promise& reject(std::any&& value);

    //! \todo add synchronization
    //! \note Sync method.
    std::any value() const      { return _d->result; };
    //! \todo add synchronization
    //! \note Sync method.
    template< typename _T >
    _T value() const            { return std::move(std::any_cast<_T>(_d->result)); };

    //! \note Sync method.
    bool isFulFilled() const    { return _d->isFulFilled(); };
    //! \note Sync method.
    bool isResolved() const     { return _d->isResolved(); };
    //! \note Sync method.
    bool isRejected() const     { return _d->isRejected(); };
    //! \note Sync method.
    bool isAborted() const      { return _d->isAborted(); };
    //! \note Sync method.
    bool isFinished() const     { return _d->isFinished(); };



private:
    PrivateType _d;


    template <
        typename _Resolve,
        typename _Context
    >
    Promise _then(_Resolve&& onResolve, _Context context);

    //! \return outer promise assuming it was already set.
    //Promise _outer() const      { assert(_d.constData()->outer); return _d.constData()->outer; }

    //! \return promise value without any sync.
    const std::any& _valueLocal() const { return _d->result; }


    // handler calls

    void _handleFulfilled();
    void _handleBindInner(Promise inner, CGull::WaitType waitType);
    void _handleBindOuter(Promise outer);
    void _handleAbort();
    void _handleDeleteThis();
    void _handleSetFinisher(CGull::CallbackFunctor&& callback, bool isResolve);
    template< typename _T >
    void _handleFulfill(_T&& value, bool isResolve);
    template< typename _T >
    void _handleFulfill(const _T& value, bool isResolve);



private:
    // ====  helpers  ====


    //! Exception catcher.
    template< typename _Callback >
    void _wrapRescue(_Callback&& callback);



    //! Wraps finisher callback's return type.
    template< typename _Callback >
    void _wrapCallbackReturn(_Callback& callback);

    //! \note lambda [](...) -> void
    template< typename _Callback >
    void _wrapCallbackReturn(_Callback& callback, CGull::guts::return_void_tag);

    //! \note lambda [](...) -> std::any
    template< typename _Callback >
    void _wrapCallbackReturn(_Callback& callback, CGull::guts::return_any_tag);

    //! \note lambda [](...) -> auto
    template< typename _Callback >
    void _wrapCallbackReturn(_Callback& callback, CGull::guts::return_auto_tag);

    //! \note lambda [](...) -> Promise
    template< typename _Callback >
    void _wrapCallbackReturn(_Callback& callback, CGull::guts::return_promise_tag);



    //! Wraps finisher callback's argumets.
    template<
        typename _Callback,
        typename _CallbackTraits = CGull::guts::function_traits<_Callback>
    >
    auto _wrapCallbackArgs(_Callback& callback)
        -> typename _CallbackTraits::result_type;

    //! \note lambda [](void) -> auto
    template<
        typename _Callback,
        typename _CallbackTraits = CGull::guts::function_traits<_Callback>
    >
    auto _wrapCallbackArgs(_Callback& callback, CGull::guts::args_count_0)
        -> typename _CallbackTraits::result_type;

    //! \note lambda [](std::any) -> auto
    template<
        typename _Callback,
        typename _CallbackTraits = CGull::guts::function_traits<_Callback>
    >
    auto _wrapCallbackArgs(_Callback& callback, CGull::guts::args_count_1_any)
        -> typename _CallbackTraits::result_type;

    //! \note lambda [](auto) -> auto
    template<
        typename _Callback,
        typename _CallbackTraits = CGull::guts::function_traits<_Callback>
    >
    auto _wrapCallbackArgs(_Callback& callback, CGull::guts::args_count_1_auto)
        -> typename _CallbackTraits::result_type;



    //! Removes std::any_cast exception
    template<
        typename _T,
        typename _dT = std::decay_t<_T>
    >
    static _dT _unwrapArg(std::any&& value);

};


#include "promise.hpp"

#if defined(CGULL_USE_NAMESPACE)
};
#endif


namespace CGull::guts
{

    // template<> struct function_return_value_traits< Promise >
    template<>
    struct function_return_value_traits< Promise >
    { using tag = return_promise_tag; };

};



#if defined(CGULL_QT)

#   include <QtGlobal>

#   if defined(CGULL_USE_NAMESPACE)

        namespace CGull
        {
            using PromiseList = QList<Promise>;
        };

        Q_DECLARE_TYPEINFO(CGull::Promise, Q_MOVABLE_TYPE);
        Q_DECLARE_METATYPE(CGull::Promise);
        Q_DECLARE_TYPEINFO(CGull::PromiseList, Q_MOVABLE_TYPE);
        Q_DECLARE_METATYPE(CGull::PromiseList);

#   else

        using PromiseList = QList<Promise>;

        Q_DECLARE_TYPEINFO(Promise, Q_MOVABLE_TYPE);
        Q_DECLARE_METATYPE(Promise);
        Q_DECLARE_TYPEINFO(PromiseList, Q_MOVABLE_TYPE);
        Q_DECLARE_METATYPE(PromiseList);

#   endif

#endif
