#pragma once

#include <cassert>

#define CGULL_DEBUG_GUTS

#if defined(CGULL_DEBUG_GUTS)
#   include <iostream>
#endif

#include "common.h"
#include "handler.h"


namespace CGull::guts
{
    class PromisePrivate;
};


namespace CGull::guts
{
    CGULL_DECLARE_ALLOCATOR(PromisePrivate);
    CGULL_DEFINE_ALLOCATOR(PromisePrivate);


    class /*CGULL_API*/ PromisePrivate : public shared_data
    {
        CGULL_USE_ALLOCATOR(PromisePrivate);

        PromisePrivate(const PromisePrivate&) = delete;
        PromisePrivate(PromisePrivate&&) = delete;
        PromisePrivate& operator=(const PromisePrivate&) = delete;
        PromisePrivate& operator=(PromisePrivate&&) = delete;

    public:
        using Type = shared_data_ptr<PromisePrivate>;


        std::any            result;

        AtomicPromiseState  state   = { CGull::NotFinished };
        AtomicWaitType      wait    = { CGull::All };

        Type                outer;
        std::vector<Type>   inners;

        CallbackFunctor     finisher;

        Handler*            handler = nullptr;

#if defined(CGULL_DEBUG_GUTS)
        PromisePrivate()
        {
            std::cout << "PromisePrivate::_ctor()\n";
        }

        ~PromisePrivate()
        {
            std::cout << "PromisePrivate::_dtor()\n";
        }
#else
        PromisePrivate() = default;
#endif


    };

};

//CGULL_EXTERN template class CGULL_API CGull::guts::shared_data_ptr<CGull::guts::PromisePrivate>;
//CGULL_EXTERN template class CGULL_API std::vector< CGull::guts::shared_data_ptr<CGull::guts::PromisePrivate> >;
