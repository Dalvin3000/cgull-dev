// Copyright 2018 Vladislav Yaremenko
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.


namespace CGull::guts
{

#if defined(CGULL_DEBUG_GUTS)

    inline
    PromisePrivate::PromisePrivate()
    {
        _DebugPromiseList::instance().add(this);
        this->_debugIdx = _debugCounter.fetch_add(1);

        Log() 
            << "@" << _debugIdxStr(this)
            << " " << "PromisePrivate::_ctor()"
            << "\n";
#if 0
        Log()
            << "PromisePrivate::_ctor() -- "
            << std::dec << (++_debugPrivateCounter())
            << " -- " << std::hex << this << std::dec
            << "\n";
#endif
    }


    inline
    PromisePrivate::~PromisePrivate()
    {
        handler->deleteHandlerData(handlerData);

        Log()
            << "@" << _debugIdxStr(this)
            << " " << "PromisePrivate::_dtor()"
            << "\n";
#if 0
        Log()
            << "PromisePrivate::_dtor() -- "
            << std::dec << (--_debugPrivateCounter())
            << " -- " << std::hex << this << std::dec
            << "\n";
#endif
        _DebugPromiseList::instance().remove(this);
    }

#endif


    inline
    void PromisePrivate::setFinisherLocal(CallbackFunctor&& callback, bool isResolve)
    {
#if CGULL_DEBUG_GUTS
        Log()
            << "@" << _debugIdxStr(this)
            << " " << "PromisePrivate::setFinisherLocal"
            << "\n";
#endif

        const auto st = finishState;

        assert(!st && "Promise already finished.");
        assert(!finisher && "Can't set finisher twice.");

        if(st)
            return;

        finisher = std::move(callback);
        finisherIsResolver = isResolve;
        finishState = CGull::Awaiting;

        // Call finisher only if promise resolved
        if(fulfillmentState)
            tryFinishLocal();
    }


    inline
    void PromisePrivate::bindInnerLocal(Type inner, CGull::WaitType waitType)
    {
#if CGULL_DEBUG_GUTS
        Log()
            << "@" << _debugIdxStr(this)
            << " " << "PromisePrivate::bindInnerLocal"
            << " " << std::hex << inner.get() << std::dec
            << " " << WaitTypeStr(waitType)
            << "\n";
#endif

        inners.push_back(inner);
        wait = waitType;
    }


    inline
    void PromisePrivate::bindOuterLocal(Type outer)
    {
#if CGULL_DEBUG_GUTS
        Log()
            << "@" << _debugIdxStr(this)
            << " " << "PromisePrivate::bindOuterLocal"
            << " " << _debugIdxStr(outer.get())
            << "\n";
#endif

        if(isFulFilled())
            outer->handler->tryFinish(outer);
        else
            outers.push_back(outer);
    }


    inline
    void PromisePrivate::fulfillLocal(std::any&& value, CGull::FulfillmentState state)
    {
#if CGULL_DEBUG_GUTS
        Log()
            << "@" << _debugIdxStr(this)
            << " " << "PromisePrivate::fulfillLocal"
            << " " << FulfillmentStateStr(state)
            << "\n";
#endif

        auto tmp = CGull::NotFulfilled;

        if(fulfillmentState.compare_exchange_strong(tmp, CGull::FulfillingNow, std::memory_order_acquire))
        {
            result = std::move(value);
            fulfillmentState.store(state);

            if(finishState < CGull::Thenned)
                finishState = CGull::Skipped;

            _propagate();
        };
    }


    inline
    void PromisePrivate::tryFinishLocal()
    {
#if CGULL_DEBUG_GUTS
        Log()
            << "@" << _debugIdxStr(this)
            << " " << "PromisePrivate::tryFinishLocal"
            << "\n";
#endif

        const auto fnState = finishState;

        // check if promise already finished
        if(fnState >= CGull::Thenned && isFulFilled())
            return;

        // check inners for outer promise or just return if promise not chained
        if(inners.size())
        {
            auto [innersFFState, innersFFResult] = _checkInnersFulfillment();

            if(innersFFState < CGull::Resolved)
                return;

            // finish or try propagate to outer. skip if not fulfilled
            if( (fnState == CGull::Awaiting && innersFFState == CGull::Resolved && finisherIsResolver) ||
                (fnState == CGull::Awaiting && innersFFState == CGull::Rejected && !finisherIsResolver))
            {
                assert(!!finisher && "tryFinish() can't be called without callback set.");

                finisher(CGull::Execute, std::move(innersFFResult)); // not async
                finisher = nullptr;
            }
            else
            {
                unbindInners();
                fulfillLocal(std::move(innersFFResult), innersFFState);
                abortLocal();
            };
        };
    }


    inline
    void PromisePrivate::completeFinish()
    {
#if CGULL_DEBUG_GUTS
        Log()
            << "@" << _debugIdxStr(this)
            << " " << "PromisePrivate::completeFinish"
            << "\n";
#endif

        finishState = finisherIsResolver ? CGull::Thenned : CGull::Rescued;

        unbindInners();
    }


    inline
    void PromisePrivate::abortLocal()
    {
#if CGULL_DEBUG_GUTS
        Log()
            << "@" << _debugIdxStr(this)
            << " " << "PromisePrivate::abortLocal"
            << "\n";

#endif
        if(finisher)
        {
            finisher(CGull::Abort, std::move(std::any{}));
            finisher = nullptr;
        };

        if(finishState < CGull::Thenned)
            finishState = CGull::Skipped;

        unbindInners();
        _propagate();
    }


    inline
    void PromisePrivate::unbindInners()
    {
#if CGULL_DEBUG_GUTS
        Log()
            << "@" << _debugIdxStr(this)
            << " " << "PromisePrivate::unbindInners"
            << "\n";
#endif

        if (!inners.empty())
#if PROMISE_USE_STD_SHARED
            std::swap(inners, PromisePrivateWeakList{});
#else
            std::swap(inners, PromisePrivateList{});
#endif
    }


    inline
    void PromisePrivate::unbindOuters()
    {
#if CGULL_DEBUG_GUTS
        Log()
            << "@" << _debugIdxStr(this)
            << " " << "PromisePrivate::unbindOuters"
            << "\n";
#endif
        if(!outers.empty())
            std::swap(outers, PromisePrivateList{});
    }


    inline
    bool PromisePrivate::deleteThisLocal()
    {
#if CGULL_DEBUG_GUTS
        Log()
            << "@" << _debugIdxStr(this)
            << " " << "PromisePrivate::deleteThisLocal"
            << "\n";
#endif

#if PROMISE_USE_STD_SHARED
        const auto refs = shared_from_this().use_count();

        assert(refs);

        if ((refs == 2 + outers.size() && inners.empty()) //< check for root promise
            || refs <= 1)                              //< last ref
        {
            if (!isFulFilled())
                fulfillLocal(std::move(std::any{}), CGull::Aborted);

            return true;
        };
#else
        const auto refs = _ref.load();

        assert(refs);

        if ((refs == 2 + outers.size() && inners.empty()) //< check for root promise
            || refs <= 1)                              //< last ref
        {
            if (!isFulFilled())
                fulfillLocal(std::move(std::any{}), CGull::Aborted);

            return true;
        };
#endif
        

        return false;
    }


    inline
    std::tuple<CGull::FulfillmentState, std::any> PromisePrivate::_checkInnersFulfillment()
    {
#if CGULL_DEBUG_GUTS
        Log()
            << "@" << _debugIdxStr(this)
            << " " << "PromisePrivate::_checkInnersFulfillment"
            << "\n";
#endif

        const auto wt = wait.load();
        const auto innersCount = inners.size();

        switch(wt)
        {
        default:
                throw std::logic_error("CGull::Promise: wait type not implemented");
        case CGull::Any:
            {
                bool somethingNotFinished = false;

                for (const auto & i : inners) 
                {
#if PROMISE_USE_STD_SHARED
                    const auto inn = i.lock();
#else
                    const auto inn = i;
#endif
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

                for (const auto & i : inners)
                {
#if PROMISE_USE_STD_SHARED
                    const auto inn = i.lock();
#else
                    const auto inn = i;
#endif
                    results.emplace_back(inn->result);
                }
                    

                return { CGull::Rejected, std::any{std::move(results)} };
            }
        case CGull::All:
            {
                bool somethingNotFinished = false;

                for (const auto & i : inners)
                {
#if PROMISE_USE_STD_SHARED
                    const auto inn = i.lock();
#else
                    const auto inn = i;
#endif
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

                for (const auto & i : inners)
                {
#if PROMISE_USE_STD_SHARED
                    results.emplace_back(i.lock()->result);
#else
                    results.emplace_back(i->result);
#endif
                }
                return { CGull::Resolved, std::any{std::move(results)} };
            }
        case CGull::First:
            {
                for(const auto & i : inners)
                {
#if PROMISE_USE_STD_SHARED
                    const auto inn = i.lock();
#else
                    const auto inn = i;
#endif
                    if(inn->isFulFilled())
                        return { inn->fulfillmentState, inn->result };
                };

                return { CGull::NotFulfilled, std::any{} };
            }
        case CGull::LastBound:
            {
#if PROMISE_USE_STD_SHARED
            const auto inn = inners.back().lock();
#else
            const auto inn = inners.back();
#endif

                if(!inn->isFulFilled())
                    return { CGull::NotFulfilled, std::any{} };

                return { inn->fulfillmentState, inn->result };
            }
        };

        return { CGull::NotFulfilled, std::any{} };
    }


    inline
    void PromisePrivate::_propagate()
    {
#if CGULL_DEBUG_GUTS
        Log()
            << "@" << _debugIdxStr(this)
            << " " << "PromisePrivate::_propagate"
            << "\n";
#endif

        for(const auto outer : outers)
            outer->handler->tryFinish(outer);

        unbindOuters();
    }
};
