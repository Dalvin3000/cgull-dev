// Copyright 2018 Vladislav Yaremenko
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

#pragma once

#include <functional>
#include <typeinfo>
#include <type_traits>
#include <tuple>
#include <any>

#if defined(CGULL_QT)
#include <QThread>
#endif


namespace CGull::guts
{
    //! \return [value] true if type \a _T is one of \a _OtherT types.
    template<typename _T, typename ... _OtherT>
    struct one_of_types : std::disjunction< std::is_same<_OtherT, _T> ... >::type {};

    //! Function return value tagging
    struct return_auto_tag {};
    struct return_void_tag {};
    struct return_any_tag {};
    struct return_promise_tag {};

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

    // template<> struct function_return_value_traits< Promise > ...


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


    //! Member function type info helper.
    template<
        typename _Class,
        typename _Result,
        typename ... _Args
    >
    struct memfn_type
    {
        using type =
            typename std::conditional<
                std::is_const<_Class>::value,
                typename std::conditional<
                    std::is_volatile<_Class>::value,
                    _Result (_Class::*)(_Args...) const volatile,
                    _Result (_Class::*)(_Args...) const
                >::type,
                typename std::conditional<
                    std::is_volatile<_Class>::value,
                    _Result (_Class::*)(_Args...) volatile,
                    _Result (_Class::*)(_Args...)
                >::type
            >::type;
    };



    //! General function type info.
    template<typename T>
    struct function_traits : public function_traits<decltype(&T::operator())> { };

    template<
        typename _Result,
        typename ... _Args
    >
    struct function_traits<_Result(_Args...)>
    {
        //! Full type of function.
        using function_type = _Result (_Args...);

        //! Full type of member function.
        template<typename _Class>
        using member_type =
            typename memfn_type<
                typename std::remove_pointer<
                typename std::remove_reference<_Class>::type>::type,
                _Result,
                _Args...
            >::type;

        //! Type of return value.
        using result_type = _Result;

        //! Return value tag.
        using result_tag = typename function_return_value_traits< result_type >::tag;

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
        struct arg_safe< I, typename std::enable_if< _arg_safe_condition_helper< I >::value >::type >
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


    template< typename _T >
    using memfn_internal = std::_Mem_fn<_T>;



#if defined(CGULL_QT)
    template<>
    struct function_traits<QThread*> { };
#endif

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


    template<typename _Function>
    struct function_traits<std::function<_Function> > : public function_traits<_Function> { };

    template<typename _Result, typename _Class>
    struct function_traits<memfn_internal<_Result _Class::*> > : public function_traits<_Result(_Class*)> { };

    template<typename _Result, typename _Class, typename ... _Args>
    struct function_traits<memfn_internal<_Result(_Class::*)(_Args...)> > : public function_traits<_Result(_Class*, _Args...)> { };

    template<typename _Result, typename _Class, typename ... _Args>
    struct function_traits<memfn_internal<_Result(_Class::*)(_Args...) const> > : public function_traits<_Result(const _Class*, _Args...)> { };

    template<typename _Result, typename _Class, typename ... _Args>
    struct function_traits<memfn_internal<_Result(_Class::*)(_Args...) volatile> > : public function_traits<_Result(volatile _Class*, _Args...)> { };

    template<typename _Result, typename _Class, typename ... _Args>
    struct function_traits<memfn_internal<_Result(_Class::*)(_Args...) const volatile> > : public function_traits<_Result(const volatile _Class*, _Args...)> { };


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
};
