#pragma once

#include "function_traits.h"
#include "promise_private.h"
#include "sync_handler.h"

#if defined(CGULL_USE_NAMESPACE)
namespace CGull
{
#endif



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

    Promise resolve(const std::any& value);
    Promise resolve(std::any&& value);

    Promise reject(const std::any& value);
    Promise reject(std::any&& value);


private:
    PrivateType _d;


    template <
        typename _Resolve,
        typename _Context
    >
    Promise _then(_Resolve&& onResolve, _Context context);

    Promise _outer() const
    {
        auto D = _d.constData();

        if(D->outer)
            return D->outer;

        return Promise{};
    }

    void _handleFulfilled();
    void _handleBindInner(Promise inner, CGull::WaitType waitType);
    void _handleAbort();
    void _handleDeleteThis();
    void _handleSetFinisher(CGull::CallbackFunctor&& callback, bool isResolve);
    template< typename _T >
    void _handleFulfill(_T&& value, bool isResolve);
    template< typename _T >
    void _handleFulfill(const _T& value, bool isResolve);


    //! \return promise value without any sync.
    const std::any& _valueLocal()
    { return _d->result; }


private:
    // ====  helpers  ====


    //! Exception catcher.
    template< typename _Callback >
    void _wrapRescue(_Callback callback);


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
    template< typename _T >
    static const _T& _unwrapArg(const std::any& value);
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
