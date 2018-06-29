#pragma once

#define _ENABLE_ATOMIC_ALIGNMENT_FIX
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS true
#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING true

#include <Promise>

#include <deque>
#include <chrono>
#include <boost/lockfree/queue.hpp>

#if defined(CGULL_DEBUG_GUTS)
#   define CHECK_CGULL_PROMISE_GUTS \
    EXPECT_EQ(0, CGull::guts::_debugPrivateCounter()); \
    CGull::guts::_debugPrivateCounter().store(0); \
    Log() << '\n';
#else
#   define CHECK_CGULL_PROMISE_GUTS
#endif


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

#if defined(CGULL_DEBUG_GUTS)
auto _debugPromiseList = &CGull::guts::_DebugPromiseList::instance();
#endif

#define WAIT_FOR(s, t) \
    for(int i = 0; i < (s); ++i) { if(t()) break; std::this_thread::sleep_for(std::chrono::milliseconds(1)); }


#define CHAINV chain += v

class Log
{
    Log(const Log&) = delete;
    Log(Log&&) = delete;
    Log& operator=(const Log&) = delete;
    Log& operator=(Log&&) = delete;

public:
    Log()
    {
#if defined(CGULL_DEBUG_GUTS)
        CGull::guts::_debugCoutMutex().lock();
#endif
    }
    ~Log()
    {
#if defined(CGULL_DEBUG_GUTS)
        CGull::guts::_debugCoutMutex().unlock();
#endif
    }

    template< typename _T >
    Log& operator<<(const _T& data)
    {
        std::cout << data;
        return *this;
    }
};


class ChainTestHelper
{
public:
    boost::lockfree::queue<int> results;
    std::deque<int>             expectedResults;


    ChainTestHelper(std::initializer_list<int> _expectedResults)
        : expectedResults{ _expectedResults }
        , results{ 64 }
    { }

    void append(int v) { results.push(v); Log() << '=' << v << '\n'; }
    ChainTestHelper& operator+=(int v) { append(v); return *this; }

    uint64_t isFailed()
    {
        auto            expected = expectedResults;
        std::deque<int> actual;
        uint64_t        expectedMask = 0;
        uint64_t        actualMask = 0;

        while(!results.empty())
        {
            int v;
            results.pop(v);
            actual.push_back(v);
        };

        for(size_t i = 0, ie = actual.size(); i != ie; ++i)
            actualMask |= 1ULL << i;
        for(size_t i = 0, ie = expected.size(); i != ie; ++i)
            expectedMask |= 1ULL << i;

        if(actualMask != expectedMask)
            return actualMask;

        if(actual != expected)
            return actualMask;

        return 0;
    }
};


TEST(guts__ref_counter, base)
{
    using namespace CGull::guts;

    RefCounter v;

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


TEST(guts__shared_data, base)
{
    using namespace CGull::guts;

    SharedData v;

    EXPECT_EQ(0, v._ref);

    v._ref.ref();

    EXPECT_EQ(1, v._ref);

    SharedData v2{v};

    EXPECT_EQ(0, v2._ref);
}


TEST(guts, static_asserts)
{
    EXPECT_EQ(sizeof(int), sizeof(CGull::guts::SharedData));
    EXPECT_EQ(sizeof(intptr_t), sizeof(CGull::guts::SharedDataPtr<CGull::guts::SharedData>));

    EXPECT_EQ(sizeof(int8_t), sizeof(std::atomic<CGull::FinishState>));
}


TEST(guts__shared_data_ptr, ref_count)
{
    using namespace CGull::guts;

    struct TestStruct : public SharedData
    {
        static int& d() { static int v = 0; return v; }
        TestStruct()    { ++d(); }
        ~TestStruct()   { --d(); }
    };

    EXPECT_EQ(0, TestStruct::d());

    {
        SharedDataPtr<TestStruct> val(new TestStruct);

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
    CGull::SyncHandler::useForThisThread();

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

    CHECK_CGULL_PROMISE_GUTS;
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
    using Log = CGull::guts::Log;

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

        Log() << type_name< function_traits< decltype(c) >::arg_safe<0>::type >() << '\n';

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

    Log() << TEST_OUT( sizeof(std::shared_ptr<QThread>) ) << '\n'<< '\n';

    Log() << TEST_OUT( sizeof(QSharedData) ) << '\n';
    Log() << TEST_OUT( sizeof(QExplicitlySharedDataPointer<QSharedData>) ) << '\n'<< '\n';

    Log() << TEST_OUT( sizeof(CGull::guts::SharedData) ) << '\n';
    Log() << TEST_OUT( sizeof(CGull::guts::SharedDataPtr<CGull::guts::SharedData>) ) << '\n'<< '\n';

    Log() << TEST_OUT( sizeof(std::deque<intptr_t>) ) << '\n';
    Log() << TEST_OUT( sizeof(std::vector<intptr_t>) ) << '\n';
    Log() << TEST_OUT( sizeof(QList<intptr_t>) ) << '\n';
    Log() << TEST_OUT( sizeof(QListData) ) << '\n'<< '\n';

    Log() << TEST_OUT( sizeof(std::any) ) << '\n';
    Log() << TEST_OUT( sizeof(QVariant::Private) ) << '\n';
    Log() << TEST_OUT( sizeof(QVariant) )  << '\n'<< '\n';

    Log() << TEST_OUT( sizeof(CGull::guts::PromisePrivate) )  << '\n';
    Log() << TEST_OUT(sizeof(CGull::guts::PromisePrivate::result))  << '\n';
    Log() << TEST_OUT(sizeof(CGull::guts::PromisePrivate::finisher))  << '\n';
    Log() << TEST_OUT(sizeof(CGull::guts::PromisePrivate::inners))  << '\n';
    Log() << TEST_OUT(sizeof(CGull::guts::PromisePrivate::outer))  << '\n';
    Log() << TEST_OUT(sizeof(CGull::guts::PromisePrivate::handler))  << '\n';
    Log() << TEST_OUT(sizeof(CGull::guts::PromisePrivate::state))  << '\n';
    Log() << TEST_OUT(sizeof(CGull::guts::PromisePrivate::wait))  << '\n';
    Log() << TEST_OUT( sizeof(CGull::guts::CallbackFunctor) )  << '\n'<< '\n';

    std::atomic_uint a = 0;

    Log() << a.fetch_sub(1) << '\n'; Log() << a.fetch_add(1)<< '\n';

    CGull::guts::SharedDataPtr<CGull::guts::PromisePrivate> testv(new CGull::guts::PromisePrivate());

    Log() << TEST_OUT( sizeof(CGull::guts::SharedData) ) << '\n';
    Log() << TEST_OUT( sizeof(CGull::guts::SharedDataPtr<CGull::guts::SharedData>) ) << '\n'<< '\n';
};
#endif

TEST(PromiseBase, construct)
{
    {
        CGull::SyncHandler::useForThisThread();

        Promise a;

        Promise b = a;

        Promise c = std::move(b);
    }

    CHECK_CGULL_PROMISE_GUTS;
};


TEST(PromiseBase, fulfill_simple)
{
    CGull::SyncHandler::useForThisThread();

    {
        Promise p; p.resolve(2316);

        EXPECT_EQ(2316 , p.value<int>() );
        EXPECT_EQ(true , p.isResolved() );
        EXPECT_EQ(false, p.isRejected() );
        EXPECT_EQ(true , p.isFinished() );
        EXPECT_EQ(true , p.isFulFilled());
    }
    CHECK_CGULL_PROMISE_GUTS;

    {
        Promise p; p.reject(2317);

        EXPECT_EQ(2317 , p.value<int>() );
        EXPECT_EQ(false, p.isResolved() );
        EXPECT_EQ(true , p.isRejected() );
        EXPECT_EQ(true , p.isFinished() );
        EXPECT_EQ(true , p.isFulFilled());
    }
    CHECK_CGULL_PROMISE_GUTS;

    {
        std::atomic_int thenCalled = 0;

        Promise p1;
        Promise p2 = p1.resolve(2318)
            .then([&](int v) { ++thenCalled; EXPECT_EQ(2318, v); Log() << '=' << v << '\n'; });

        EXPECT_EQ(1, thenCalled);

        EXPECT_EQ(2318 , p1.value<int>() );
        EXPECT_EQ(true , p1.isResolved() );
        EXPECT_EQ(false, p1.isRejected() );
        EXPECT_EQ(true , p1.isFinished() );
        EXPECT_EQ(true , p1.isFulFilled());

        EXPECT_EQ(false, p2.value().has_value() );
        EXPECT_EQ(true , p2.isResolved() );
        EXPECT_EQ(false, p2.isRejected() );
        EXPECT_EQ(true , p2.isFinished() );
        EXPECT_EQ(true , p2.isFulFilled());
    }
    CHECK_CGULL_PROMISE_GUTS;

    {
        std::atomic_int thenCalled = 0;

        Promise p1;
        Promise p2 = p1.reject(2319)
            .then([&](int v) { ++thenCalled; EXPECT_EQ(2319, v); Log() << '=' << v << '\n'; });

        EXPECT_EQ(0, thenCalled);

        EXPECT_EQ(2319 , p1.value<int>() );
        EXPECT_EQ(false, p1.isResolved() );
        EXPECT_EQ(true , p1.isRejected() );
        EXPECT_EQ(true , p1.isFinished() );
        EXPECT_EQ(true , p1.isFulFilled());

        EXPECT_EQ(false, p2.isResolved() );
        EXPECT_EQ(true , p2.isRejected() );
        EXPECT_EQ(true , p2.isFinished() );
        EXPECT_EQ(true , p2.isFulFilled());
    }
    CHECK_CGULL_PROMISE_GUTS;

    {
        std::atomic_int thenCalled = 0;

        Promise p1;
        Promise p2 = p1.reject(2319)
            .then([&](int v) { ++thenCalled; EXPECT_EQ(2319, v); Log() << '=' << v << '\n'; return 1172; });
        Promise p3 = p2
            .then([&](int v) { ++thenCalled; EXPECT_EQ(1172, v); Log() << '=' << v << '\n'; });

        EXPECT_EQ(0, thenCalled);

        EXPECT_EQ(2319 , p1.value<int>());
        EXPECT_EQ(false, p1.isResolved());
        EXPECT_EQ(true , p1.isRejected());
        EXPECT_EQ(true , p1.isFinished());
        EXPECT_EQ(true , p1.isFulFilled());

        EXPECT_EQ(2319 , p2.value<int>());
        EXPECT_EQ(false, p2.isResolved());
        EXPECT_EQ(true , p2.isRejected());
        EXPECT_EQ(true , p2.isFinished());
        EXPECT_EQ(true , p2.isFulFilled());

        EXPECT_EQ(2319 , p3.value<int>());
        EXPECT_EQ(false, p3.isResolved());
        EXPECT_EQ(true , p3.isRejected());
        EXPECT_EQ(true , p3.isFinished());
        EXPECT_EQ(true , p3.isFulFilled());

    }
    CHECK_CGULL_PROMISE_GUTS;

    {
        std::atomic_int thenCalled = 0;

        Promise p1;
        Promise p2 = p1.reject(2319)
            .then([&](int v)   { ++thenCalled; EXPECT_EQ(2319, v); Log() << '=' << v << '\n'; return 1172; });
        Promise p3 = p2
            .rescue([&](int v) { ++thenCalled; EXPECT_EQ(2319, v); Log() << '=' << v << '\n'; return 1132; });
        Promise p4 = p3
            .then([&](int v)   { ++thenCalled; EXPECT_EQ(1132, v); Log() << '=' << v << '\n'; });

        EXPECT_EQ(2, thenCalled);

        EXPECT_EQ(2319 , p1.value<int>());
        EXPECT_EQ(false, p1.isResolved());
        EXPECT_EQ(true , p1.isRejected());
        EXPECT_EQ(true , p1.isFinished());
        EXPECT_EQ(true , p1.isFulFilled());

        EXPECT_EQ(2319 , p2.value<int>());
        EXPECT_EQ(false, p2.isResolved());
        EXPECT_EQ(true , p2.isRejected());
        EXPECT_EQ(true , p2.isFinished());
        EXPECT_EQ(true , p2.isFulFilled());

        EXPECT_EQ(1132, p3.value<int>());
        EXPECT_EQ(true , p3.isResolved());
        EXPECT_EQ(false, p3.isRejected());
        EXPECT_EQ(true , p3.isFinished());
        EXPECT_EQ(true , p3.isFulFilled());

        EXPECT_EQ(true , p4.isResolved());
        EXPECT_EQ(false, p4.isRejected());
        EXPECT_EQ(true , p4.isFinished());
        EXPECT_EQ(true , p4.isFulFilled());

    }
    CHECK_CGULL_PROMISE_GUTS;

    {
        std::atomic_int thenCalled = 0;

        Promise p1;
        Promise p2 = p1.resolve(2319)
            .then([&](int v)   { ++thenCalled; EXPECT_EQ(2319, v); Log() << '=' << v << '\n'; throw 1178; return 1172; });
        Promise p3 = p2
            .rescue([&](int v) { ++thenCalled; EXPECT_EQ(1178, v); Log() << '=' << v << '\n'; return 1132; });
        Promise p4 = p3
            .then([&](int v)   { ++thenCalled; EXPECT_EQ(1132, v); Log() << '=' << v << '\n'; });

        EXPECT_EQ(3, thenCalled);

        EXPECT_EQ(2319 , p1.value<int>());
        EXPECT_EQ(true , p1.isResolved());
        EXPECT_EQ(false, p1.isRejected());
        EXPECT_EQ(true , p1.isFinished());
        EXPECT_EQ(true , p1.isFulFilled());

        EXPECT_EQ(1178 , p2.value<int>());
        EXPECT_EQ(false, p2.isResolved());
        EXPECT_EQ(true , p2.isRejected());
        EXPECT_EQ(true , p2.isFinished());
        EXPECT_EQ(true , p2.isFulFilled());

        EXPECT_EQ(1132, p3.value<int>());
        EXPECT_EQ(true , p3.isResolved());
        EXPECT_EQ(false, p3.isRejected());
        EXPECT_EQ(true , p3.isFinished());
        EXPECT_EQ(true , p3.isFulFilled());

        EXPECT_EQ(true , p4.isResolved());
        EXPECT_EQ(false, p4.isRejected());
        EXPECT_EQ(true , p4.isFinished());
        EXPECT_EQ(true , p4.isFulFilled());

    }
    CHECK_CGULL_PROMISE_GUTS;

    {
        std::atomic_int thenCalled = 0;

        Promise p1;
        Promise p2 = p1.resolve(2319)
            .then([&](int v)   { ++thenCalled; EXPECT_EQ(2319, v); Log() << '=' << v << '\n'; throw 1178; return 1172; });
        Promise p3 = p2
            .then([&](int v)   { ++thenCalled; EXPECT_EQ(1172, v); Log() << '=' << v << '\n'; });
        Promise p4 = p3
            .rescue([&](int v) { ++thenCalled; EXPECT_EQ(1178, v); Log() << '=' << v << '\n'; return 1132; });
        Promise p5 = p4
            .then([&](int v)   { ++thenCalled; EXPECT_EQ(1132, v); Log() << '=' << v << '\n'; });

        EXPECT_EQ(3, thenCalled);

        EXPECT_EQ(2319 , p1.value<int>());
        EXPECT_EQ(true , p1.isResolved());
        EXPECT_EQ(false, p1.isRejected());
        EXPECT_EQ(true , p1.isFinished());
        EXPECT_EQ(true , p1.isFulFilled());

        EXPECT_EQ(1178 , p2.value<int>());
        EXPECT_EQ(false, p2.isResolved());
        EXPECT_EQ(true , p2.isRejected());
        EXPECT_EQ(true , p2.isFinished());
        EXPECT_EQ(true , p2.isFulFilled());

        EXPECT_EQ(1178 , p3.value<int>());
        EXPECT_EQ(false, p3.isResolved());
        EXPECT_EQ(true , p3.isRejected());
        EXPECT_EQ(true , p3.isFinished());
        EXPECT_EQ(true , p3.isFulFilled());

        EXPECT_EQ(1132 , p4.value<int>());
        EXPECT_EQ(true , p4.isResolved());
        EXPECT_EQ(false, p4.isRejected());
        EXPECT_EQ(true , p4.isFinished());
        EXPECT_EQ(true , p4.isFulFilled());

        EXPECT_EQ(true , p5.isResolved());
        EXPECT_EQ(false, p5.isRejected());
        EXPECT_EQ(true , p5.isFinished());
        EXPECT_EQ(true , p5.isFulFilled());

    }
    CHECK_CGULL_PROMISE_GUTS;

};


TEST(PromiseBase, then_simple)
{
    {
        std::atomic_int thenCalled = 0;

        CGull::SyncHandler::useForThisThread();

        Promise a;
        Promise b = a
            .then([&](int v) { thenCalled.fetch_or(0x01); EXPECT_EQ(5, v); Log() << '=' << v << '\n'; return 4; })
            .then([&](int v) { thenCalled.fetch_or(0x02); EXPECT_EQ(4, v); Log() << '=' << v << '\n'; return 7; })
            .then([&](int v) { thenCalled.fetch_or(0x04); EXPECT_EQ(7, v); Log() << '=' << v << '\n'; })
            ;

        a.resolve(5);

        EXPECT_EQ(0x07, thenCalled);

        Log() << '\n';
    }
    CHECK_CGULL_PROMISE_GUTS;

    {
        std::atomic_int thenCalled = 0;

        CGull::SyncHandler::useForThisThread();

        Promise a;
        Promise b = a
            .then([&](int v) { thenCalled.fetch_or(0x01); EXPECT_EQ(5, v); Log() << '=' << v << '\n'; return 4; })
            ;
        a.resolve(5);

        b = b.then([&](int v) { thenCalled.fetch_or(0x02); EXPECT_EQ(4, v); Log() << '=' << v << '\n'; return 7; })
            .then([&](int v) { thenCalled.fetch_or(0x04); EXPECT_EQ(7, v); Log() << '=' << v << '\n'; })
            ;

        EXPECT_EQ(0x07, thenCalled);

        Log() << '\n';
    }
    CHECK_CGULL_PROMISE_GUTS;

    {
        std::atomic_int thenCalled = 0;

        CGull::SyncHandler::useForThisThread();

        Promise a;
        Promise b = a.resolve(5)
            .then([&](int v) { thenCalled.fetch_or(0x01); EXPECT_EQ(5, v); Log() << '=' << v << '\n'; return 4; })
            .then([&](int v) { thenCalled.fetch_or(0x02); EXPECT_EQ(4, v); Log() << '=' << v << '\n'; return 7; })
            .then([&](int v) { thenCalled.fetch_or(0x04); EXPECT_EQ(7, v); Log() << '=' << v << '\n'; })
            ;

        EXPECT_EQ(0x07, thenCalled);

        Log() << '\n';
    }
    CHECK_CGULL_PROMISE_GUTS;

    {
        std::atomic_int thenCalled = 0;

        CGull::SyncHandler::useForThisThread();

        Promise b = Promise{}.resolve(5)
            .then([&](int v) { thenCalled.fetch_or(0x01); EXPECT_EQ(5, v); Log() << '=' << v << '\n'; return 4; })
            .then([&](int v) { thenCalled.fetch_or(0x02); EXPECT_EQ(4, v); Log() << '=' << v << '\n'; return 7; })
            .then([&](int v) { thenCalled.fetch_or(0x04); EXPECT_EQ(7, v); Log() << '=' << v << '\n'; })
            ;

        EXPECT_EQ(0x07, thenCalled);

        Log() << '\n';
    }
    CHECK_CGULL_PROMISE_GUTS;

};


TEST(PromiseBase, nested_thens)
{
    CGull::SyncHandler::useForThisThread();

    {
        CGull::SyncHandler::useForThisThread();
        ChainTestHelper chain = {5,4,7};

        Promise a, c, b = a
            .then([&](int v) { CHAINV; return c; })
            .then([&](int v) { CHAINV; return 7; })
            .then([&](int v) { CHAINV; });

        a.resolve(5);
        c.resolve(4);

        EXPECT_EQ(0, chain.isFailed());
    }

    CHECK_CGULL_PROMISE_GUTS; Log() << '\n';

    {
        CGull::SyncHandler::useForThisThread();
        ChainTestHelper chain = { 5,4,7 };

        Promise a, c, b = a
            .then([&](int v) { CHAINV; return c; })
            .then([&](int v) { CHAINV; return 7; })
            .then([&](int v) { CHAINV; });

        c.resolve(4);
        a.resolve(5);

        EXPECT_EQ(0, chain.isFailed());
    }

    CHECK_CGULL_PROMISE_GUTS; Log() << '\n';

    {
        CGull::SyncHandler::useForThisThread();
        ChainTestHelper chain = { 5,4,7 };

        Promise a, b = a
            .then([&](int v) { CHAINV; return Promise{}.resolve(4); })
            .then([&](int v) { CHAINV; return 7; })
            .then([&](int v) { CHAINV; });

        a.resolve(5);

        EXPECT_EQ(0, chain.isFailed());
    }

    CHECK_CGULL_PROMISE_GUTS; Log() << '\n';

    {
        CGull::SyncHandler::useForThisThread();
        ChainTestHelper chain = { 5,4,7 };

        Promise a, c, b = a
            .then([&](int v) { CHAINV; return c.resolve(4); })
            .then([&](int v) { CHAINV; return 7; })
            .then([&](int v) { CHAINV; });

        a.resolve(5);

        EXPECT_EQ(0, chain.isFailed());
    }

    CHECK_CGULL_PROMISE_GUTS; Log() << '\n';




    {
        CGull::SyncHandler::useForThisThread();
        ChainTestHelper chain = { 5,4,9,7 };

        Promise a, c, d, b = a
            .then([&](int v) { CHAINV; return d = c
                .then([&](int v) { CHAINV; return 9; });
                })
            .then([&](int v) { CHAINV; return 7; })
            .then([&](int v) { CHAINV; });

        c.resolve(4);
        a.resolve(5);

        EXPECT_EQ(0, chain.isFailed());
    }

    CHECK_CGULL_PROMISE_GUTS; Log() << '\n';

    {
        CGull::SyncHandler::useForThisThread();
        ChainTestHelper chain = { 5,4,9,7 };

        Promise a, c, d, b = a
            .then([&](int v) { CHAINV; return d = c
                .then([&](int v) { CHAINV; return 9; });
                })
            .then([&](int v) { CHAINV; return 7; })
            .then([&](int v) { CHAINV; });

        a.resolve(5);
        c.resolve(4);

        EXPECT_EQ(0, chain.isFailed());
    }

    CHECK_CGULL_PROMISE_GUTS; Log() << '\n';

    {
        CGull::SyncHandler::useForThisThread();
        ChainTestHelper chain = { 5,4,9,7 };

        Promise a, c, d, b = a
            .then([&](int v) { CHAINV; return d = c.resolve(4)
                .then([&](int v) { CHAINV; return 9; });
                })
            .then([&](int v) { CHAINV; return 7; })
            .then([&](int v) { CHAINV; });

        a.resolve(5);

        EXPECT_EQ(0, chain.isFailed());
    }

    CHECK_CGULL_PROMISE_GUTS; Log() << '\n';

    {
        CGull::SyncHandler::useForThisThread();
        ChainTestHelper chain = { 5,4,9,7 };

        Promise a, c, d, b = a.resolve(5)
            .then([&](int v) { CHAINV; return d = c.resolve(4)
                .then([&](int v) { CHAINV; return 9; });
                })
            .then([&](int v) { CHAINV; return 7; })
            .then([&](int v) { CHAINV; });

        a;

        EXPECT_EQ(0, chain.isFailed());
    }

    CHECK_CGULL_PROMISE_GUTS; Log() << '\n';

    {
        CGull::SyncHandler::useForThisThread();
        ChainTestHelper chain = { 5,4,9,7 };

        Promise a, c, d, b = a.resolve(5)
            .then([&](int v) { CHAINV; return d = c
                .then([&](int v) { CHAINV; return 9; });
                })
            .then([&](int v) { CHAINV; return 7; })
            .then([&](int v) { CHAINV; });

        a;
        c.resolve(4);

        EXPECT_EQ(0, chain.isFailed());
    }

    CHECK_CGULL_PROMISE_GUTS; Log() << '\n';

    {
        CGull::SyncHandler::useForThisThread();
        ChainTestHelper chain = { 5,4,9,7 };

        Promise a, c, d, b = a
            .then([&](int v) { CHAINV; return d = c.resolve(4)
                .then([&](int v) { CHAINV; return 9; });
                })
            .then([&](int v) { CHAINV; return 7; }); a.resolve(5); b
            .then([&](int v) { CHAINV; });

        a;
        c;

        EXPECT_EQ(0, chain.isFailed());
    }

    CHECK_CGULL_PROMISE_GUTS; Log() << '\n';

    {
        CGull::SyncHandler::useForThisThread();
        ChainTestHelper chain = { 5,4,9,7 };

        Promise a, c, d, b = a
            .then([&](int v) { CHAINV; return d = c
                .then([&](int v) { CHAINV; return 9; });
                })
            .then([&](int v) { CHAINV; return 7; }); a.resolve(5); b
            .then([&](int v) { CHAINV; });

        a;
        c.resolve(4);

        EXPECT_EQ(0, chain.isFailed());
    }

    CHECK_CGULL_PROMISE_GUTS; Log() << '\n';

}


TEST(PromiseBase, several_thens)
{
    CGull::SyncHandler::useForThisThread();

    /*
    a = new Promise((r)=>{r(5);});
    a.then((r)=>console.log(r)).then(()=>console.log(1));
    a.then((r)=>console.log(r)).then(()=>console.log(2));
    VM362:2 5
    VM362:3 5
    VM362:2 1
    VM362:3 2
    */

    {
        std::atomic_int thenCalled = 0;

        Promise a;

        a
            .then([&](int v) { EXPECT_EQ(312, v); thenCalled.fetch_add(1); return 444; })
            .then([&](int v) { EXPECT_EQ(444, v); thenCalled.fetch_add(2); });

        a
            .then([&](int v) { EXPECT_EQ(312, v); thenCalled.fetch_add(3); return 555; })
            .then([&](int v) { EXPECT_EQ(555, v); thenCalled.fetch_add(4); });

        a.resolve(312);

        EXPECT_EQ(10, thenCalled);
    }
    CHECK_CGULL_PROMISE_GUTS;

    {
        std::atomic_int thenCalled = 0;

        Promise a;

        a.resolve(312);

        a
            .then([&](int v) { EXPECT_EQ(312, v); thenCalled.fetch_add(1); return 444; })
            .then([&](int v) { EXPECT_EQ(444, v); thenCalled.fetch_add(2); });

        a
            .then([&](int v) { EXPECT_EQ(312, v); thenCalled.fetch_add(3); return 555; })
            .then([&](int v) { EXPECT_EQ(555, v); thenCalled.fetch_add(4); });

        EXPECT_EQ(10, thenCalled);
    }
    CHECK_CGULL_PROMISE_GUTS;

    {
        std::atomic_int thenCalled = 0;

        Promise a;

        a
            .then([&](int v) { EXPECT_EQ(312, v); thenCalled.fetch_add(1); return 444; })
            .then([&](int v) { EXPECT_EQ(444, v); thenCalled.fetch_add(2); });

        a.resolve(312);

        a
            .then([&](int v) { EXPECT_EQ(312, v); thenCalled.fetch_add(3); return 555; })
            .then([&](int v) { EXPECT_EQ(555, v); thenCalled.fetch_add(4); });

        EXPECT_EQ(10, thenCalled);
    }
    CHECK_CGULL_PROMISE_GUTS;

    {
        std::atomic_int thenCalled = 0;

        Promise a;

        auto b = a.then([&](int v) { EXPECT_EQ(312, v); thenCalled.fetch_add(1); return 444; });

        a.resolve(312);

        b.then([&](int v) { EXPECT_EQ(444, v); thenCalled.fetch_add(2); });

        a
            .then([&](int v) { EXPECT_EQ(312, v); thenCalled.fetch_add(3); return 555; })
            .then([&](int v) { EXPECT_EQ(555, v); thenCalled.fetch_add(4); });

        EXPECT_EQ(10, thenCalled);
    }
    CHECK_CGULL_PROMISE_GUTS;

    {
        std::atomic_int thenCalled = 0;

        Promise a;

        a
            .then([&](int v) { EXPECT_EQ(312, v); thenCalled.fetch_add(3); return 555; })
            .then([&](int v) { EXPECT_EQ(555, v); thenCalled.fetch_add(4); });

        auto b = a.then([&](int v) { EXPECT_EQ(312, v); thenCalled.fetch_add(1); return 444; });

        a.resolve(312);

        b.then([&](int v) { EXPECT_EQ(444, v); thenCalled.fetch_add(2); });

        EXPECT_EQ(10, thenCalled);
    }
    CHECK_CGULL_PROMISE_GUTS;
};


TEST(PromiseBase, abort)
{
    CGull::SyncHandler::useForThisThread();

    {
        std::atomic_int thenCalled = 0;

        Promise a;

        a
            .then([&]() { thenCalled.fetch_add(1); })
            .then([&]() { thenCalled.fetch_add(1); })
            .rescue([&]() { thenCalled.fetch_add(1); });

        a
            .then([&]() { thenCalled.fetch_add(1); })
            .rescue([&]() { thenCalled.fetch_add(1); })
            .then([&]() { thenCalled.fetch_add(1); });

        EXPECT_EQ(0, thenCalled);
    }

    CHECK_CGULL_PROMISE_GUTS;

    {
        Promise{};
    }

    CHECK_CGULL_PROMISE_GUTS;

    {
        std::atomic_int thenCalled = 0;

        Promise{}
            .then([&]() { thenCalled.store(1); });

        EXPECT_EQ(0, thenCalled);
    }

    CHECK_CGULL_PROMISE_GUTS;

    {
        std::atomic_int thenCalled = 0;

        Promise{}
            .then([&]() { thenCalled.fetch_add(1); })
            .then([&]() { thenCalled.fetch_add(1); });

        EXPECT_EQ(0, thenCalled);
    }

    CHECK_CGULL_PROMISE_GUTS;
};


TEST(PromiseBase, then_compilation)
{
    {
        CGull::SyncHandler::useForThisThread();

        { auto next = Promise{}.resolve(1).then([]() {}); };
        { auto next = Promise{}.resolve(1).then([](const std::any&) {}); };
        { auto next = Promise{}.resolve(1).then([](std::any) {}); };
        { auto next = Promise{}.resolve(1).then([](std::any&&) {}); };
        { auto next = Promise{}.resolve(1).then([](const int&) {}); };
        { auto next = Promise{}.resolve(1).then([](int) {}); };
        { auto next = Promise{}.resolve(1).then([](int&&) {}); };
        { auto next = Promise{}.resolve(1).then([](const std::string&) {}); };
        { auto next = Promise{}.resolve(1).then([](std::string) {}); };
        { auto next = Promise{}.resolve(1).then([](std::string&&) {}); };

        { auto next = Promise{}.resolve(1).then([a = 10](int prev) { Log() << prev + a << '\n'; }); };

        { auto next = Promise{}.resolve(1).then([]() { return 1; }); };
        { auto next = Promise{}.resolve(1).then([]() { return std::string{}; }); };
        { auto next = Promise{}.resolve(1).then([]() { return std::any{}; }); };
        { auto next = Promise{}.resolve(1).then([]() { return Promise{}; }); };
        { auto next = Promise{}.then([]() { return Promise{}; }); };
        { auto next = Promise{}.resolve(1).then([]() { return Promise{}.resolve(2); }); };
        { auto next = Promise{}.then([]() { return Promise{}.resolve(2); }); };
    }

    CHECK_CGULL_PROMISE_GUTS;
};


TEST(PromiseBase, rescue_compilation)
{
    {
        CGull::SyncHandler::useForThisThread();

        try
        {
            ChainTestHelper chain = {1,3};
            Promise{}.resolve(1)
                .then([&](int v)
                    { 
                        CHAINV; throw 3; return 2;
                    })
                .rescue([&](int v)
                    { 
                        CHAINV; 
                    });
            EXPECT_EQ(0, chain.isFailed());
        }
        catch(...) { EXPECT_FALSE(1); };

        try
        {
            ChainTestHelper chain = { 1,3 };
            Promise{}.resolve(1)
                .then([&](int v) { CHAINV; throw "3"; return 2; })
                .rescue([&](const char* _v) { int v = std::atoi(_v); CHAINV; });
            EXPECT_EQ(0, chain.isFailed());
        }
        catch(...) { EXPECT_FALSE(1); };

        try
        {
            ChainTestHelper chain = { 1,3 };
            Promise{}.resolve(1)
                .then([&](int v) { CHAINV; throw std::any{3}; return std::any{2}; })
                .rescue([&](std::any _v) { int v = std::any_cast<int>(_v); CHAINV; });
            EXPECT_EQ(0, chain.isFailed());
        }
        catch(...) { EXPECT_FALSE(1); };

        {
            bool catchCalled = false;
            try
            {
                ChainTestHelper chain = { 1,3 };
                Promise{}.resolve(1)
                    .then([&](int v) { CHAINV; throw 3.0f; return 2; })
                    .rescue([&](float v) { CHAINV; });
                EXPECT_EQ(0, chain.isFailed());
            }
            catch(std::runtime_error e)
            {
                catchCalled = true;
            }
            catch(...) { EXPECT_FALSE(1); };

            EXPECT_TRUE(catchCalled);
        }

        {
            bool catchCalled = false;
            try
            {
                ChainTestHelper chain = { 1,3 };
                Promise{}.resolve(1)
                    .then([&](int v) { CHAINV; throw 3.0f; })
                    .rescue([&](float v) { CHAINV; });
                EXPECT_EQ(0, chain.isFailed());
            }
            catch(std::runtime_error e)
            {
                catchCalled = true;
            }
            catch(...) { EXPECT_FALSE(1); };

            EXPECT_TRUE(catchCalled);
        }

    }

    CHECK_CGULL_PROMISE_GUTS;
};

#if 0

TEST(PromiseBase, benchmark)
{
    unsigned count{ 100000 };

    CGull::SyncHandler::useForThisThread();

    for (unsigned i=0; i<count; ++i) 
    {
        { auto next = Promise{}.resolve(1).then([]() {}); };
        { auto next = Promise{}.resolve(1).then([](const std::any&) {}); };
        { auto next = Promise{}.resolve(1).then([](std::any) {}); };
        { auto next = Promise{}.resolve(1).then([](std::any&&) {}); };
        { auto next = Promise{}.resolve(1).then([](const int&) {}); };
        { auto next = Promise{}.resolve(1).then([](int) {}); };
        { auto next = Promise{}.resolve(1).then([](int&&) {}); };
        { auto next = Promise{}.resolve(1).then([]() { return 1; }); };
        { auto next = Promise{}.resolve(1).then([]() { return std::any{}; }); };
        { auto next = Promise{}.resolve(1).then([]() { return Promise{}; }); };
        { auto next = Promise{}.then([]() { return Promise{}; }); };
        { auto next = Promise{}.resolve(1).then([]() { return Promise{}.resolve(2); }); };
        { auto next = Promise{}.then([]() { return Promise{}.resolve(2); }); };
    }
}
#endif
