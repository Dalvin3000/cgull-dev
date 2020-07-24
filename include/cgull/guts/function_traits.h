#pragma once

#include "../config.h"

#include <functional>
#include <any>
#include <tuple>


CGULL_NAMESPACE_START
CGULL_GUTS_NAMESPACE_START


//! Function return value tagging
struct return_tag {};
struct return_auto_tag : return_tag {};
struct return_void_tag : return_tag {};
struct return_any_tag : return_tag {};
struct return_promise_tag : return_tag {};

// template<> struct function_return_value_traits< ... >
template< typename _T >
struct function_return_value_traits
{ using tag = return_auto_tag; };

// template<> struct function_return_value_traits< void >
template<>
struct function_return_value_traits< void >
{ using tag = return_void_tag; };

// template<> struct function_return_value_traits< std::any >
template<>
struct function_return_value_traits< std::any >
{ using tag = return_any_tag; };

// template<> struct function_return_value_traits< promise >
//     see it in promise.h after promise definition...


//! Function arguments tagging
struct args_count_0 {};
struct args_count_1_auto {};
struct args_count_1_any {};
struct args_count_x {};

// template<> struct function_args_traits< (>1), auto >
template< int _C, typename _T >
struct function_args_traits
{ using tag = args_count_x; };

// template<> struct function_args_traits< 0, void >
template<>
struct function_args_traits< 0, void >
{ using tag = args_count_0; };

// template<> struct function_args_traits< 1, std::any >
template<>
struct function_args_traits< 1, std::any >
{ using tag = args_count_1_any; };

// template<> struct function_args_traits< 1, auto >
template< typename _X >
struct function_args_traits< 1, _X  >
{ using tag = args_count_1_auto; };


template<typename T>
struct function_traits : public function_traits<decltype(&T::operator())> {};

template<
    typename _Result,
    typename ... _Args
>
struct function_traits<_Result(_Args...)>
{
    //! Full type of function.
    using function_type = _Result(_Args...);

    //! Full type of member function.
#if 0
    template<typename _Class>
    using member_type =
        typename memfn_type<
        typename std::remove_pointer<
        typename std::remove_reference<_Class>::type>::type,
        _Result,
        _Args...
        >::type;
#endif

    //! Type of return value.
    using result_type = _Result;

    //! Return value tag.
    using result_tag = typename function_return_value_traits< result_type >::tag;

    //! Full function type fingerprint.
    using fingerprint_type = std::tuple< result_type, _Args... >;

    //! Signature for void-returning callback where result passed as first argument.
    template<typename _ThisResult, typename _PromiseType, typename _ResultFirst, typename _Enable = void>
    struct void_returning
    {
        using type = _ThisResult (_PromiseType, _ResultFirst, _Args...);
    };

    template<typename _ThisResult, typename _PromiseType, typename _ResultFirst>
    struct void_returning<_ThisResult, _PromiseType, _ResultFirst, std::enable_if_t<std::is_void_v<_ResultFirst>> >
    {
        using type = _ThisResult (_PromiseType, _Args...);
    };

    template<typename _ThisResult, typename _PromiseType>
    using void_returning_t = typename void_returning<_ThisResult, _PromiseType, _Result>::type;

    //! Arguments count.
    static constexpr int args_count = sizeof...(_Args);

    //! Arguments type info by element with OOR error generation.
    template< size_t I >
    struct arg
    {
        using type = typename std::tuple_element< I, std::tuple<_Args...> >::type;
    };

    //! Arguments type info by element with OOR error hiding.
    template< size_t I, typename _Enable = void >
    struct arg_safe
    {
        using type = void;
    };

    //! msvc enable_if type deduction bug workaround. ref: https://stackoverflow.com/questions/3209085
    template< size_t I >
    struct _arg_safe_condition_helper
    {
        static constexpr bool value = I < args_count;
    };

    template< size_t I >
    struct arg_safe< I, std::enable_if_t< _arg_safe_condition_helper< I >::value > >
    {
        using type = typename arg<I>::type;
    };

    //! Arguments traits.
    using args_tag =
        typename function_args_traits<
        args_count,
        std::decay_t< typename arg_safe<0>::type >
        >::tag;
};


template<typename _Result, typename ... _Args>
struct function_traits<_Result(*)(_Args...)> : public function_traits<_Result(_Args...)> {};

template<typename _Class, typename _Result, typename ... _Args>
struct function_traits<_Result(_Class::*)(_Args...)> : public function_traits<_Result(_Args...)>
{
    //! Type of member function class.
    using owner_type = _Class&;
};

template<typename _Class, typename _Result, typename ... _Args>
struct function_traits<_Result(_Class::*)(_Args...) const> : public function_traits<_Result(_Args...)>
{
    //! Type of member function class.
    using owner_type = const _Class&;
};

template<typename _Class, typename _Result, typename ... _Args>
struct function_traits<_Result(_Class::*)(_Args...) volatile> : public function_traits<_Result(_Args...)>
{
    //! Type of member function class.
    using owner_type = volatile _Class&;
};

template<typename _Class, typename _Result, typename ... _Args>
struct function_traits<_Result(_Class::*)(_Args...) const volatile> : public function_traits<_Result(_Args...)>
{
    //! Type of member function class.
    using owner_type = const volatile _Class&;
};


template< typename _T >
using memfn_internal = std::_Mem_fn<_T>;


template<typename _Function>
struct function_traits<std::function<_Function> > : public function_traits<_Function> {};

template<typename _Result, typename _Class>
struct function_traits<memfn_internal<_Result _Class::*> > : public function_traits<_Result(_Class*)> {};

template<typename _Result, typename _Class, typename ... _Args>
struct function_traits<memfn_internal<_Result(_Class::*)(_Args...)> > : public function_traits<_Result(_Class*, _Args...)> {};

template<typename _Result, typename _Class, typename ... _Args>
struct function_traits<memfn_internal<_Result(_Class::*)(_Args...) const> > : public function_traits<_Result(const _Class*, _Args...)> {};

template<typename _Result, typename _Class, typename ... _Args>
struct function_traits<memfn_internal<_Result(_Class::*)(_Args...) volatile> > : public function_traits<_Result(volatile _Class*, _Args...)> {};

template<typename _Result, typename _Class, typename ... _Args>
struct function_traits<memfn_internal<_Result(_Class::*)(_Args...) const volatile> > : public function_traits<_Result(const volatile _Class*, _Args...)> {};


template<typename T> struct function_traits<T&> : public function_traits<T> {};
template<typename T> struct function_traits<const T&> : public function_traits<T> {};
template<typename T> struct function_traits<volatile T&> : public function_traits<T> {};
template<typename T> struct function_traits<const volatile T&> : public function_traits<T> {};
template<typename T> struct function_traits<T&&> : public function_traits<T> {};
template<typename T> struct function_traits<const T&&> : public function_traits<T> {};
template<typename T> struct function_traits<volatile T&&> : public function_traits<T> {};
template<typename T> struct function_traits<const volatile T&&> : public function_traits<T> {};

template< typename _Callback >
using functor_t = typename std::function<typename function_traits<_Callback>::function_type>;


// argument dispatcher
template<
    int _I,
    typename _Head,
    typename... _Args
>
struct argGetter
{
    using return_type = typename argGetter<_I-1, _Args...>::return_type;

    static return_type get(_Head&&, _Args&&... tail)
    {
        return argGetter<_I-1, _Args...>::get(tail...);
    }
};

template<
    typename _Head,
    typename... _Args
>
struct argGetter<0, _Head, _Args...>
{
    using return_type = _Head;

    static return_type get(_Head&& head, _Args&&... )
    {
        return head;
    }
};

template<
    int _I,
    typename _Head,
    typename... _Args
>
auto getArg(_Head&& head, _Args&&... tail) -> typename argGetter<_I, _Head, _Args...>::return_type
{
    return argGetter<_I, _Head, _Args...>::get(head, tail...);
}


template<typename _Func1, typename _Func2>
using is_same_fingerprint =
    std::is_same<
        typename function_traits<_Func1>::fingerprint_type,
        typename function_traits<_Func2>::fingerprint_type
    >;

template<typename _Func1, typename _Func2> inline
constexpr bool is_same_fingerprint_v = is_same_fingerprint<_Func1, _Func2>::value;


CGULL_GUTS_NAMESPACE_END
CGULL_NAMESPACE_END
