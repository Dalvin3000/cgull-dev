// Copyright 2018 Vladislav Yaremenko
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

#pragma once

#include <cassert>
#include <tuple>

#if defined(CGULL_DEBUG_GUTS)
#   include <iostream>
#endif

#include "common.h"
#include "memory.h"


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


    class /*CGULL_API*/ PromisePrivate : public RefCounter<PromisePrivate>
    {
        CGULL_USE_ALLOCATOR(PromisePrivate);

        // disable copy and move
        PromisePrivate(const PromisePrivate&) = delete;
        PromisePrivate(PromisePrivate&&) = delete;
        PromisePrivate& operator=(const PromisePrivate&) = delete;
        PromisePrivate& operator=(PromisePrivate&&) = delete;

    public:
        using Type = SharedPtr<PromisePrivate>;
        using PromisePrivateList = std::vector<Type>;


        //! Context-local write and context-local read.
        std::any                result;

        //! Context-local.
        AtomicWaitType          wait                = { CGull::All };
        //! Context-local write and context-local read.
        AtomicFulfillmentState  fulfillmentState    = { CGull::NotFulfilled };
        //! Context-local write and context-local read.
        volatile FinishState    finishState         = { CGull::NotFinished };

        //! Tells whether finisher is resolver or rescuer.
        bool                    finisherIsResolver;

        //! Context-local.
        PromisePrivateList      outers;
        //! Context-local.
        PromisePrivateList      inners;

        //! Context-local.
        std::any                innersResultCache;
        //! Finisher callback for chain fulfillment. Contex-local.
        CallbackFunctor         finisher;

        //! Async handler used by promise context-remote.
        Handler* volatile       handler = nullptr;
        //! Internal data used by handler contex-local.
        void*                   handlerData = nullptr;


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

        //! Binds new outer dependency.
        void bindOuterLocal(Type outer);

        //! Resolves/Rejects this promise with value \a result.
        void fulfillLocal(std::any&& value, CGull::FulfillmentState state);

        //! Checks inner promises and calls then/rescue on this promise if condition met.
        void tryFinishLocal();
        void completeFinish();

        //! Clears inner dependencies and finisher.
        void abortLocal();

        //! Clears inner dependencies.
        void unbindInners();

        //! Clears outer dependencies and propagates result to them.
        void unbindOuters();

        //! Checks promise for utilization state.
        bool deleteThisLocal();

        bool isFulFilled() const { return fulfillmentState.load() >= CGull::Resolved; };
        bool isResolved() const  { return fulfillmentState.load() == CGull::Resolved; };
        bool isRejected() const  { return fulfillmentState.load() == CGull::Rejected; };
        bool isAborted() const   { return fulfillmentState.load() == CGull::Aborted; };
        bool isFinished() const  { return finishState >= CGull::Thenned; };


    private:
        std::tuple<CGull::FulfillmentState, std::any>
             _checkInnersFulfillment();
        void _propagate();
    };


};

//CGULL_EXTERN template class CGULL_API CGull::guts::SharedDataPtr<CGull::guts::PromisePrivate>;
//CGULL_EXTERN template class CGULL_API std::vector< CGull::guts::SharedDataPtr<CGull::guts::PromisePrivate> >;

// included in 'handler.h'
//#include "promise_private.hpp"
