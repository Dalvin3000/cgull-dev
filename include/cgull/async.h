#pragma once

#include "promise.h"


CGULL_NAMESPACE_START


template< typename ... _Args > inline
promise when_all(_Args&&... promises)
{
    //! \todo not implemented
    return guts::getArg<0>(promises...);
}


//! \note Exception/error in wrapped function that prevents callback
//!       from calling will lead to dangling functor in functors store.
template<
    typename _Function,
    typename _FallbackCallback = void (void),
    typename _Handler = void, //!< \todo not implemented
    int _IndentCbParameterOuter = 0, //!< Callback UID accessed from context-local.
    int _IndentCbParameterInner = 0, //!< Callback UID accessed from async (usually same as outer UID).
    int _IndentCallbackParameter = 0 //!< C callback. Reverse iterator. \todo Atm. works only 0.
>
struct async
{
    using value_type = guts::functor_t<_Function>;
    using traits = guts::function_traits<_Function>;
    using fallback_callback_type = guts::functor_t<typename traits::template void_returning_t<bool, promise>>;

    using callback_type = typename traits::template arg< traits::args_count-1-_IndentCallbackParameter >::type;
    using wrapped_callback_type = guts::functor_t< callback_type >;
    using store_type = std::unordered_map< std::intptr_t, wrapped_callback_type >;

    mutable value_type              _fn;
    mutable fallback_callback_type  _fb;


    //! Ctor with no fallback solution. I.e. \a fn will never fail and promise always
    //! will be resolved.
    //!
    //! \sa async(fn, fb)
    async(_Function fn)
        : _fn(value_type{ fn })
    { }

    //! Ctor with fallback functor that will check if \a fn failed and then let to reject promise
    //! manually with custom value.
    //!
    //! \param fb Functor of type bool(promise, [fn::return_type, ], fn::arg...).
    //! \note nullptr will be passed to fb instead of internal callback to prevent possible access to internal data.
    //!
    //! \sa async(fn)
    async(_Function fn, _FallbackCallback fb)
        : _fn(value_type{ fn })
        , _fb(fallback_callback_type{ fb })
    {
        static_assert(
            guts::is_same_fingerprint_v< fallback_callback_type, _FallbackCallback >,
            "Fallback callback must have signature bool(promise, [fn::return_type, ], fn::arg...)"
        );
    }

    //! \note At this point we must ensure that callback will be called
    //!       in the same thread.
    template< typename ... _Args >
    auto operator()(_Args&&... args) const
    {
        return _call(typename traits::result_tag{}, nullptr, std::forward<_Args>(args)...);
    };


    template< typename ... _Args >
    promise _call(guts::return_void_tag, typename traits::result_type* _fn_result, _Args&&... args) const
    {
        thread_local store_type store;

        promise result;

        const wrapped_callback_type wrappedCallback =
            [r = result]< typename ... _CArgs >(_CArgs&&... args) mutable -> void
            {
                //! \todo pass many args as tuple
                r.resolve(std::any{guts::getArg<0>(args...)});
            };

        const callback_type nakedCallback =
            []< typename ... _CArgs >(_CArgs... args) -> void
            {
                auto key = reinterpret_cast<std::intptr_t>(guts::getArg<_IndentCbParameterInner>(args...));

                static_assert(
                    (std::is_integral_v< decltype(key) > || std::is_pointer_v< decltype(key) >) && sizeof(decltype(key)) >= sizeof(intptr_t),
                    "To access promise from async call you need to provide integral UID for it (int, ptr, etc.)"
                );

                auto callback = store.extract(key);

                assert(!callback.empty() && "Async callback not found");

                try
                {
                    callback.mapped()(args...);
                }
                catch(...)
                {
                    assert(false && "cgull::async() doesn't support exceptions inside callabacks");
                };
            };

        auto key = reinterpret_cast<std::intptr_t>(guts::getArg<_IndentCbParameterOuter>(args...));

        static_assert(
            (std::is_integral_v< decltype(key) > || std::is_pointer_v< decltype(key) >) && sizeof(decltype(key)) >= sizeof(intptr_t),
            "To access promise from async call you need to provide integral UID for it (int, ptr, etc.)"
        );

        store[key] = wrappedCallback;

        if constexpr(std::is_void_v< typename traits::result_type >)
        {
            _fn(std::forward<_Args>(args)..., nakedCallback);

            if(_fb && _fb(result, std::forward<_Args>(args)..., nullptr))
                store.erase(key);
        }
        else
        {
            auto&& r = _fn(std::forward<_Args>(args)..., nakedCallback);

            if(_fb && _fb(result, std::forward<decltype(r)>(r), std::forward<_Args>(args)..., nullptr))
                store.erase(key);

            if(_fn_result)
                *_fn_result = r;
        };

        return result;
    };

    template< typename ... _Args >
    promise _call(guts::return_auto_tag, typename traits::result_type*, _Args&&... args) const
    {
        typename traits::result_type _fn_result;

        auto&& result = _call(guts::return_void_tag{}, &_fn_result, args...);

        std::cout << "result_of_fn=" << _fn_result << '\n';

        return when_all(result, promise{}.resolve(std::move(_fn_result)));
    };

};

CGULL_NAMESPACE_END
