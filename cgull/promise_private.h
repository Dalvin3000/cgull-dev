// Copyright 2018 Vladislav Yaremenko
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

#pragma once

#include <cassert>
#include <tuple>

#define CGULL_DEBUG_GUTS //!< \todo remove

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
        using InnersList = std::vector<Type>;


        std::any                result;
        std::any                innersResultCache;

        AtomicFinishState       finishState         = { CGull::NotFinished };
        AtomicFulfillmentState  fulfillmentState    = { CGull::NotFulfilled };
        AtomicWaitType          wait                = { CGull::All };

        InnersList              outers;
        InnersList              inners;

        CallbackFunctor         finisher;

        Handler*                handler = nullptr;

        bool    isFulFilled() const { return fulfillmentState.load() >= CGull::Resolved; };
        bool    isResolved() const  { return fulfillmentState.load() == CGull::Resolved; };
        bool    isRejected() const  { return fulfillmentState.load() == CGull::Rejected; };
        bool    isAborted() const   { return fulfillmentState.load() == CGull::Aborted; };
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

        //! Binds new outer dependency.
        void bindOuterLocal(Type outer);

        //! Resolves/Rejects this promise with value \a result.
        void fulfillLocal(std::any&& value, CGull::FulfillmentState state);

        //! Resolves/Rejects this promise if it's an outer promise.
        void tryFinishLocal();

        //! Clears inner dependencies and finisher.
        void abortLocal();

        //! Clears inner dependencies.
        void unbindInners();

        //! Clears outer dependencies and propagates result to them.
        void unbindOuters();

        //! Checks promise for utilization state.
        void deleteThisLocal();


    private:
        std::tuple<CGull::FulfillmentState, std::any>
             _checkInnersFulfillment();
        void _propagate();
    };


};

//CGULL_EXTERN template class CGULL_API CGull::guts::shared_data_ptr<CGull::guts::PromisePrivate>;
//CGULL_EXTERN template class CGULL_API std::vector< CGull::guts::shared_data_ptr<CGull::guts::PromisePrivate> >;
