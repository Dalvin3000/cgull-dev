#pragma once

#include <cassert>

#define CGULL_DEBUG_GUTS

#if defined(CGULL_DEBUG_GUTS)
#   include <iostream>
#endif

#include "common.h"


namespace CGull
{
    namespace guts
    {
        class PromisePrivate;
    };

    class Handler;
};


namespace CGull::guts
{
    CGULL_DECLARE_ALLOCATOR(PromisePrivate);
    CGULL_DEFINE_ALLOCATOR(PromisePrivate);


    class /*CGULL_API*/ PromisePrivate : public shared_data
    {
        CGULL_USE_ALLOCATOR(PromisePrivate);

        // disable copy and move
        PromisePrivate(const PromisePrivate&) = delete;
        PromisePrivate(PromisePrivate&&) = delete;
        PromisePrivate& operator=(const PromisePrivate&) = delete;
        PromisePrivate& operator=(PromisePrivate&&) = delete;

    public:
        using Type = shared_data_ptr<PromisePrivate>;


        std::any                result;

        AtomicFinishState       finishState         = { CGull::NotFinished };
        AtomicFulfillmentState  fulfillmentState    = { CGull::NotFulfilled };
        AtomicWaitType          wait                = { CGull::All };

        Type                    outer;
        std::vector<Type>       inners;

        CallbackFunctor         finisher;

        Handler*                handler = nullptr;

        bool    isFulFilled() const { return fulfillmentState.load() >= CGull::Resolved; };
        bool    isResolved() const  { return fulfillmentState.load() == CGull::Resolved; };
        bool    isRejected() const  { return fulfillmentState.load() == CGull::Rejected; };
        bool    isFinished() const  { return finishState.load() >= CGull::Thenned; };


#if defined(CGULL_DEBUG_GUTS)
        PromisePrivate();
        ~PromisePrivate();
#else
        PromisePrivate() = default;
#endif

        //! Sets then/rescue callback.
        void setFinisherLocal(CallbackFunctor&& callback, bool isResolve);

        //! Binds new inner dependency.
        void bindInnerLocal(Type inner, CGull::WaitType waitType);

        //! Resolves/Rejects this promise with value \a result.
        void fulfillLocal(std::any&& value, bool isResolve);

        //! Resolves/Rejects this promise if it's an outer promise.
        void checkFulfillmentLocal();

        //! Clears inner dependencies and finisher.
        void abortLocal();

        //! Clears inner dependencies.
        void unbindInners();


    private:
        void _checkInners();
        void _finish(CGull::FinishState fnState);
        void _propagate();
    };


};

//CGULL_EXTERN template class CGULL_API CGull::guts::shared_data_ptr<CGull::guts::PromisePrivate>;
//CGULL_EXTERN template class CGULL_API std::vector< CGull::guts::shared_data_ptr<CGull::guts::PromisePrivate> >;
