
namespace CGull::guts
{

#if defined(CGULL_DEBUG_GUTS)

    inline
    PromisePrivate::PromisePrivate()
    {
        std::cout << "PromisePrivate::_ctor()\n";
    }


    inline
    PromisePrivate::~PromisePrivate()
    {
        std::cout << "PromisePrivate::_dtor()\n";
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
            checkFulfillmentLocal();
    }


    inline
    void PromisePrivate::bindInnerLocal(Type inner, CGull::WaitType waitType)
    {
        inners.push_back(inner);
        wait = waitType;

        checkFulfillmentLocal();
    }


    inline
    void PromisePrivate::fulfillLocal(std::any&& value, bool isResolve)
    {
        auto tmp = CGull::NotFulfilled;

        if(fulfillmentState.compare_exchange_strong(tmp, CGull::FulfillingNow, std::memory_order_acquire))
        {
            result = value; // copy
            fulfillmentState.store(isResolve ? CGull::Resolved : CGull::Rejected);

            checkFulfillmentLocal();
        };
    }


    inline
    void PromisePrivate::checkFulfillmentLocal()
    {
        // check inners for outer promise or just return if promise not chained
        if(!fulfillmentState)
        {
            if(inners.size())
                _checkInners();
            else
                return;
        };

        const auto fnState = finishState.load();

        // check if promise already finished
        if(!fnState || fnState >= CGull::Thenned)
            return;

        // wait async fulfillment
        //while(fulfillmentState.load() == CGull::FulfillingNow);

        const auto ffState = fulfillmentState.load();

        // finish or try propagate to outer
        if(ffState == CGull::Resolved)
        {
            if(fnState == CGull::AwaitingResolve)
                _finish(CGull::Thenned);
            else
                _propagate();
        }
        else
        {
            if(fnState == CGull::AwaitingReject)
                _finish(CGull::Rescued);
            else
                _propagate();
        };
    }


    inline
    void PromisePrivate::abortLocal()
    {
        if(finisher)
        {
            finisher(CGull::Abort);
            finisher = nullptr;
        };

        unbindInners();
    }


    inline
    void PromisePrivate::unbindInners()
    {
        for(size_t i = 0, ie = inners.size(); i < ie; ++i)
            inners.at(i)->outer.reset();
    }


    inline
    void PromisePrivate::_checkInners()
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

                    if(!inn->isFulFilled()) // don't need to check finish at this moment
                        somethingNotFinished = true;
                    else if(inn->isResolved())
                    {
                        // resolved inner found
                        result = inn->result;
                        fulfillmentState.store(CGull::Resolved);

                        return;
                    };
                };

                // still have not finished and fulfilled inners
                if(somethingNotFinished)
                    return;

                // reject on no resolved promise
                CGull::AnyList results;

                results.reserve(innersCount);

                for(int i = 0; i < innersCount; ++i)
                    results.emplace_back(inners.at(i)->result);

                result = results;
                fulfillmentState.store(CGull::Rejected);

                return;
            }
        case CGull::All:
            {
                bool somethingNotFinished = false;

                for(int i = 0; i < innersCount; ++i)
                {
                    const auto inn = inners.at(i);

                    if(inn->isRejected())
                    {
                        // one of inners rejected
                        result = inn->result;
                        fulfillmentState.store(CGull::Rejected);

                        return;
                    }
                    else if(!inn->isFinished())
                        somethingNotFinished = true;
                };

                // must wait all promises to resolve
                if(somethingNotFinished)
                    return;

                // resolve on all resolved promises
                CGull::AnyList results;

                results.reserve(innersCount);

                for(int i = 0; i < innersCount; ++i)
                    results.emplace_back(inners.at(i)->result);

                result = results;
                fulfillmentState.store(CGull::Resolved);

                return;
            }
        case CGull::First:
            {
                for(int i = 0; i < innersCount; ++i)
                {
                    const auto inn = inners.at(i);

                    if(inn->isFinished())
                    {
                        result = inn->result;
                        fulfillmentState.store(inn->fulfillmentState);

                        return;
                    };
                };

                return;
            }
        case CGull::LastBound:
            {
                const auto inn = inners.back();

                if(!inn->isFinished())
                    return;

                result = inn->result;
                fulfillmentState.store(inn->fulfillmentState);

                return;
            }
        };
    }


    inline
    void PromisePrivate::_finish(CGull::FinishState fnState)
    {
        assert((bool)finisher);

        finisher(CGull::Execute);
        finishState.store(fnState);
    }


    inline
    void PromisePrivate::_propagate()
    {
        if(outer)
            outer->handler->fulfilled(outer);
    }
};
