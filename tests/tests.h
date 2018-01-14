#pragma once

#include <Promise>


#if 1
template< typename _T >
std::string type_name()
{
    using _TNoReference = typename std::remove_reference<_T>::type;

#if defined(_MSC_VER)
    std::string r = typeid(_TNoReference).name();
#else
    auto demangled = abi::__cxa_demangle(typeid(_TNoReference).name(), nullptr, nullptr, nullptr);

    std::string r = demangled ? demangled : std::string{};

    std::free(demangled);
#endif

    if(std::is_volatile<_TNoReference>::value)   r += " volatile";
    if(std::is_const<_TNoReference>::value)      r += " const";

    if(std::is_lvalue_reference<_T>::value)      r += "&";
    else if(std::is_rvalue_reference<_T>::value) r += "&&";

    return std::move(r);
}
#endif


TEST(guts__ref_counter, base)
{
    using namespace CGull::guts;

    ref_counter v;

    EXPECT_EQ(0, v);

    EXPECT_EQ(true, v.ref());
    EXPECT_EQ(1, v);

    EXPECT_EQ(true, v.ref());
    EXPECT_EQ(2, v);

    EXPECT_EQ(true, v.deref());
    EXPECT_EQ(1, v);

    EXPECT_EQ(true, v.ref());
    EXPECT_EQ(2, v);

    EXPECT_EQ(true, v.deref());
    EXPECT_EQ(1, v);

    EXPECT_EQ(false, v.deref());
    EXPECT_EQ(0, v);
}


TEST(guts, static_asserts)
{
    EXPECT_EQ(sizeof(int), sizeof(CGull::guts::shared_data));
    EXPECT_EQ(sizeof(intptr_t), sizeof(CGull::guts::shared_data_ptr<CGull::guts::shared_data>));

    EXPECT_EQ(sizeof(int), sizeof(std::atomic<CGull::PromiseState>));
}


TEST(guts__shared_data_ptr, ref_count)
{
    using namespace CGull::guts;

    struct TestStruct : public shared_data
    {
        static int& d() { static int v = 0; return v; }
        TestStruct()    { ++d(); }
        ~TestStruct()   { --d(); }
    };

    EXPECT_EQ(0, TestStruct::d());

    {
        shared_data_ptr<TestStruct> val(new TestStruct);

        EXPECT_EQ(1, TestStruct::d());
        EXPECT_EQ(1, val.constData()->_ref.load());

        {
            const auto val2 = val;

            EXPECT_EQ(1, TestStruct::d());
            EXPECT_EQ(2, val .constData()->_ref.load());
            EXPECT_EQ(2, val2.constData()->_ref.load());
        }

        EXPECT_EQ(1, TestStruct::d());
        EXPECT_EQ(1, val.constData()->_ref.load());
    }

    EXPECT_EQ(0, TestStruct::d());
}


template< typename _Cb >
auto guts__callback_traits__return_tags__fn(_Cb /*cb*/, CGull::guts::return_auto_tag) { return int{}; }

template< typename _Cb >
auto guts__callback_traits__return_tags__fn(_Cb /*cb*/, CGull::guts::return_void_tag) { return int64_t{}; }

template< typename _Cb >
auto guts__callback_traits__return_tags__fn(_Cb /*cb*/, CGull::guts::return_any_tag) { return std::any{}; }

template< typename _Cb >
auto guts__callback_traits__return_tags__fn(_Cb /*cb*/, CGull::guts::return_promise_tag) { return Promise{}; }

template< typename _Cb >
auto guts__callback_traits__return_tags__fn(_Cb cb)
{ return guts__callback_traits__return_tags__fn(cb, typename CGull::guts::function_traits<_Cb>::result_tag{}); }

TEST(guts__callback_traits, static_asserts__return_tags)
{
    {
        auto r = guts__callback_traits__return_tags__fn([]() { return 0UL; });
        static_assert(std::is_same< decltype(r), int >::value, "`function_traits<>::result_tag` default doesn't work");
    }
    {
        auto r = guts__callback_traits__return_tags__fn([]() { });
        static_assert(std::is_same< decltype(r), int64_t >::value, "`function_traits<>::result_tag` for `void` doesn't work");
    }
    {
        auto r = guts__callback_traits__return_tags__fn([]() { return std::any{}; });
        static_assert(std::is_same< decltype(r), std::any >::value, "`function_traits<>::result_tag` for `any` doesn't work");
    }
    {
        auto r = guts__callback_traits__return_tags__fn([]() { return Promise{}; });
        static_assert(std::is_same< decltype(r), Promise >::value, "`function_traits<>::result_tag` for `Promise` doesn't work");
    }
}


template< typename _Cb >
auto guts__callback_traits__args_tags__fn(_Cb /*cb*/, CGull::guts::args_count_0) { return int{}; }

template< typename _Cb >
auto guts__callback_traits__args_tags__fn(_Cb /*cb*/, CGull::guts::args_count_1_any) { return int64_t{}; }

template< typename _Cb >
auto guts__callback_traits__args_tags__fn(_Cb /*cb*/, CGull::guts::args_count_1_auto) { return short{}; }

template< typename _Cb >
auto guts__callback_traits__args_tags__fn(_Cb /*cb*/, CGull::guts::args_count_x) { return float{}; }

template< typename _Cb >
auto guts__callback_traits__args_tags__fn(_Cb cb)
{ return guts__callback_traits__args_tags__fn(cb, typename CGull::guts::function_traits<_Cb>::args_tag{}); }

TEST(guts__callback_traits, static_asserts__args_tags)
{
    using namespace CGull::guts;

    {
        auto r = guts__callback_traits__args_tags__fn([](void){});
        static_assert(std::is_same< decltype(r), int >::value, "`function_traits<>::args_tag` for zero arguments doesn't work");
    }
    {
        function_args_traits<1, std::any>::tag R;
        static_assert(std::is_same< decltype(R), args_count_1_any >::value);

        auto c = [](std::any) {};

        static_assert( function_traits< decltype(c) >::args_count == 1 );

        static_assert(
            std::is_same< typename function_traits< decltype(c) >::arg<0>::type, std::any >::value,
            "`function_traits<>::arg<>::type` for doesn't work"
        );
        static_assert(
            std::is_same< typename function_traits< decltype(c) >::arg_safe<0>::type, std::any >::value,
            "`function_traits<>::arg_safe<>::type` for doesn't work"
        );
        static_assert(
            std::is_same< typename function_traits< decltype(c) >::args_tag, args_count_1_any >::value,
            "`function_traits<>::args_tag` for one `std::any` argument doesn't work"
        );

        std::cout << type_name< function_traits< decltype(c) >::arg_safe<0>::type >() << '\n';

        auto r = guts__callback_traits__args_tags__fn([](std::any){});
        static_assert(std::is_same< decltype(r), int64_t >::value, "`function_traits<>::args_tag` for one `std::any` argument doesn't work");
    }
    {
        function_args_traits<1, int>::tag R;
        static_assert(std::is_same< decltype(R), args_count_1_auto >::value);

        auto r = guts__callback_traits__args_tags__fn([](int){});
        static_assert(std::is_same< decltype(r), short >::value, "`function_traits<>::args_tag` for one default argument doesn't work");
    }
    {
        auto r = guts__callback_traits__args_tags__fn([](int, int){});
        static_assert(std::is_same< decltype(r), float >::value, "`function_traits<>::args_tag` for many arguments doesn't work");
    }
}


#if 0
#include <QList>
#include <QVariant>
#include <vector>
#include <deque>
#include <QSharedData>

#define _TOS_TNoReference(x) #x
#define TOS_TNoReference(x) _TOS_TNoReference(x)


TEST(Promise, test)
{
#define TEST_OUT(x)  TOS_TNoReference(x) << ": " << (x)

    std::cout << TEST_OUT( sizeof(std::shared_ptr<QThread>) ) << '\n'<< '\n';

    std::cout << TEST_OUT( sizeof(QSharedData) ) << '\n';
    std::cout << TEST_OUT( sizeof(QExplicitlySharedDataPointer<QSharedData>) ) << '\n'<< '\n';

    std::cout << TEST_OUT( sizeof(CGull::guts::shared_data) ) << '\n';
    std::cout << TEST_OUT( sizeof(CGull::guts::shared_data_ptr<CGull::guts::shared_data>) ) << '\n'<< '\n';

    std::cout << TEST_OUT( sizeof(std::deque<intptr_t>) ) << '\n';
    std::cout << TEST_OUT( sizeof(std::vector<intptr_t>) ) << '\n';
    std::cout << TEST_OUT( sizeof(QList<intptr_t>) ) << '\n';
    std::cout << TEST_OUT( sizeof(QListData) ) << '\n'<< '\n';

    std::cout << TEST_OUT( sizeof(std::any) ) << '\n';
    std::cout << TEST_OUT( sizeof(QVariant::Private) ) << '\n';
    std::cout << TEST_OUT( sizeof(QVariant) )  << '\n'<< '\n';

    std::cout << TEST_OUT( sizeof(CGull::guts::PromisePrivate) )  << '\n';
    std::cout << TEST_OUT(sizeof(CGull::guts::PromisePrivate::result))  << '\n';
    std::cout << TEST_OUT(sizeof(CGull::guts::PromisePrivate::finisher))  << '\n';
    std::cout << TEST_OUT(sizeof(CGull::guts::PromisePrivate::inners))  << '\n';
    std::cout << TEST_OUT(sizeof(CGull::guts::PromisePrivate::outer))  << '\n';
    std::cout << TEST_OUT(sizeof(CGull::guts::PromisePrivate::handler))  << '\n';
    std::cout << TEST_OUT(sizeof(CGull::guts::PromisePrivate::state))  << '\n';
    std::cout << TEST_OUT(sizeof(CGull::guts::PromisePrivate::wait))  << '\n';
    std::cout << TEST_OUT( sizeof(CGull::guts::CallbackFunctor) )  << '\n'<< '\n';

    std::atomic_uint a = 0;

    std::cout << a.fetch_sub(1) << '\n'; std::cout << a.fetch_add(1)<< '\n';

    CGull::guts::shared_data_ptr<CGull::guts::PromisePrivate> testv(new CGull::guts::PromisePrivate());

    std::cout << TEST_OUT( sizeof(CGull::guts::shared_data) ) << '\n';
    std::cout << TEST_OUT( sizeof(CGull::guts::shared_data_ptr<CGull::guts::shared_data>) ) << '\n'<< '\n';
};
#endif

TEST(Promise, construct)
{
    Promise a;

    Promise b = a;

    Promise c = std::move(b);
};


TEST(Promise, then)
{
    auto next = Promise{}.then(
        [a = 10](int prev) { std::cout << prev + a; }
    );
};
