// Copyright 2018 Vladislav Yaremenko
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.


namespace CGull::guts
{

#if defined(CGULL_DEBUG_GUTS)

    inline
    PromisePrivate::PromisePrivate()
    {
        std::cout
            << "PromisePrivate::_ctor() -- "
            << std::dec << (++_debugPrivateCounter())
            << " -- " << std::hex << this << std::dec
            << "\n";
    }


    inline
    PromisePrivate::~PromisePrivate()
    {
        std::cout
            << "PromisePrivate::_dtor() -- "
            << std::dec << (--_debugPrivateCounter())
            << " -- " << std::hex << this << std::dec
            << "\n";
    }

#endif


    inline
    void PromisePrivate::setFinisherLocal(CallbackFunctor&& callback, bool isResolve)
    {
        const auto st = finishState.load();

        // can't set finisher twice
        assert(!st);

        if(st)
            return;

        finisher = std::move(callback);
        finishState.store(isResolve ? CGull::AwaitingResolve : CGull::AwaitingReject);

        // Call finisher only if promise resolved
        if(fulfillmentState)
            tryFinishLocal();
    }


    inline
    void PromisePrivate::bindInnerLocal(Type inner, CGull::WaitType waitType)
    {
        inners.push_back(inner);
        wait = waitType;

        //tryFinishLocal();
    }


    inline
    void PromisePrivate::bindOuterLocal(Type _outer)
    {
        outer = _outer;

        if(isFulFilled())
            _propagate();
    }


    inline
    void PromisePrivate::fulfillLocal(std::any&& value, CGull::FulfillmentState state)
    {
        auto tmp = CGull::NotFulfilled;

        if(fulfillmentState.compare_exchange_strong(tmp, CGull::FulfillingNow, std::memory_order_acquire))
        {
            unbindInners();

            result = std::move(value);

            if(state == CGull::Resolved)
            {
                fulfillmentState.store(CGull::Resolved);

                if(finishState == CGull::AwaitingResolve)
                    finishState.store(CGull::Thenned);
                else
                    abortLocal();
            }
            else if(state == CGull::Rejected)
            {
                fulfillmentState.store(CGull::Rejected);

                if(finishState == CGull::AwaitingReject)
                    finishState.store(CGull::Rescued);
                else
                    abortLocal();
            }
            else
                fulfillmentState.store(CGull::Aborted);

            if(outer)
                _propagate();
        };
    }


    inline
    void PromisePrivate::tryFinishLocal()
    {
        const auto fnState = finishState.load();

        // check if promise already finished
        if(fnState >= CGull::Thenned)
            return;

        // check inners for outer promise or just return if promise not chained
        if(inners.size())
        {
            auto [innersFFState, innersFFResult] = _checkInnersFulfillment();

            // finish or try propagate to outer. skip if not fulfilled
            if(innersFFState == CGull::Resolved)
            {
                if(fnState == CGull::AwaitingResolve)
                    finisher(CGull::Execute, std::move(innersFFResult));
                else
                {
                    fulfillLocal(std::move(innersFFResult), CGull::Resolved);
                    abortLocal();
                };
            }
            else if(innersFFState == CGull::Rejected)
            {
                if(fnState == CGull::AwaitingReject)
                    finisher(CGull::Execute, std::move(innersFFResult));
                else
                {
                    fulfillLocal(std::move(innersFFResult), CGull::Rejected);
                    abortLocal();
                };
            }
            else if(innersFFState == CGull::Aborted)
            {
                fulfillLocal(std::move(innersFFResult), CGull::Aborted);
                abortLocal();
            };
        };
    }


    //inline
    //void PromisePrivate::_finishLocal(CGull::FinishState fnState, std::any&& innersResult)
    //{
    //    assert((bool)finisher);

    //    finisher(CGull::Execute, std::move(innersResult));
    //}


    inline
    void PromisePrivate::_propagate()
    {
        if(outer)
        {
            outer->handler->checkFulfillment(outer);
            outer.reset();
        };
    }


    inline
    void PromisePrivate::abortLocal()
    {
        if(finisher)
        {
            finisher(CGull::Abort, std::move(std::any{}));
            finisher = nullptr;
        };

        if(finishState < CGull::Thenned)
            finishState.store(CGull::Skipped);

        unbindInners();
    }


    inline
    void PromisePrivate::unbindInners()
    {
        if(!inners.empty())
            inners = InnersList{};
    }


    inline
    void PromisePrivate::unbindOuter()
    {
        _propagate();
    }


    inline
    std::tuple<CGull::FulfillmentState, std::any> PromisePrivate::_checkInnersFulfillment()
    {
        const auto wt = wait.load();
        const auto innersCount = inners.size();

        switch(wt)
        {
        default:
                throw std::logic_error("CGull::Promise: wait type not implemented");
        case CGull::Any:
            {
                bool somethingNotFinished = false;

                for(int i = 0; i < innersCount; ++i)
                {
                    const auto inn = inners.at(i);

                    if(!inn->isFulFilled())    // don't need to check finish at this moment
                        somethingNotFinished = true;
                    else if(inn->isResolved()) // resolved inner found
                        return { CGull::Resolved, inn->result };
                };

                // still have not finished and fulfilled inners
                if(somethingNotFinished)
                    return { CGull::NotFulfilled, std::any{} };

                // reject on no resolved promise
                CGull::AnyList results;

                results.reserve(innersCount);

                for(int i = 0; i < innersCount; ++i)
                    results.emplace_back(inners.at(i)->result);

                return { CGull::Rejected, std::any{std::move(results)} };
            }
        case CGull::All:
            {
                bool somethingNotFinished = false;

                for(int i = 0; i < innersCount; ++i)
                {
                    const auto inn = inners.at(i);

                    if(inn->isRejected())   // one of inners rejected
                        return { CGull::Rejected, inn->result };
                    else if(!inn->isFulFilled())
                        somethingNotFinished = true;
                };

                // must wait all promises to resolve
                if(somethingNotFinished)
                    return { CGull::NotFulfilled, std::any{} };

                // resolve on all resolved promises
                CGull::AnyList results;

                results.reserve(innersCount);

                for(int i = 0; i < innersCount; ++i)
                    results.emplace_back(inners.at(i)->result);

                return { CGull::Resolved, std::any{std::move(results)} };
            }
        case CGull::First:
            {
                for(int i = 0; i < innersCount; ++i)
                {
                    const auto inn = inners.at(i);

                    if(inn->isFulFilled())
                        return { inn->fulfillmentState, inn->result };
                };

                return { CGull::NotFulfilled, std::any{} };
            }
        case CGull::LastBound:
            {
                const auto inn = inners.back();

                if(!inn->isFulFilled())
                    return { CGull::NotFulfilled, std::any{} };

                return { inn->fulfillmentState, inn->result };
            }
        };

        return { CGull::NotFulfilled, std::any{} };
    }
};
