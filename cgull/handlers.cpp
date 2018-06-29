// Copyright 2018 Vladislav Yaremenko
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

#include "sync_handler.h"
#include "std_thread_handler.h"
#include "promise.h"


namespace CGull
{
    namespace guts
    {
#if defined(CGULL_DEBUG_GUTS)
#endif
    };

    Handler* Handler::forThisThread()
    {
        auto const h = _threadHandler().get();

        assert(h);

        return h;
    }


    std::unique_ptr<Handler>& Handler::_threadHandler()
    {
        thread_local static std::unique_ptr<Handler> h = 0;

        return h;
    }


    // =====  SyncHandler  =====

    SyncHandler::~SyncHandler()
    {
    }

    void SyncHandler::setFinisher(PrivateType self, CallbackFunctor&& callback, bool isResolve)
    {
        self->setFinisherLocal(std::forward<CallbackFunctor>(callback), isResolve);
    }

    void SyncHandler::bindInner(PrivateType self, PrivateType inner, CGull::WaitType waitType)
    {
        self->bindInnerLocal(inner, waitType);
    }

    void SyncHandler::bindOuter(PrivateType self, PrivateType outer)
    {
        self->bindOuterLocal(outer);
    }

    void SyncHandler::tryFinish(PrivateType self)
    {
        self->tryFinishLocal();
    }

    void SyncHandler::abort(PrivateType self)
    {
        self->abortLocal();
    }

    void SyncHandler::fulfill(PrivateType self, std::any&& value, bool isResolve)
    {
        self->fulfillLocal(std::forward<std::any>(value), isResolve ? CGull::Resolved : CGull::Rejected);
    }

    void SyncHandler::init(PrivateType self)
    {
    }

    void SyncHandler::deleteThis(PrivateType self)
    {
        self->deleteThisLocal();
    }

    void SyncHandler::deleteHandlerData(void* data)
    {
    }

    void SyncHandler::useForThisThread()
    {
        if(!_threadHandler())
            _threadHandler().reset(new SyncHandler());
    }


    // =====  StdThreadHandler  =====
#define STH_HANDLER reinterpret_cast<StdThreadHandlerData*>(self->handlerData)
#define STH_THREAD_HANDLER_ASSERTION_CHECK \
    [d = STH_HANDLER](){ return \
        d->thread.get_id() == std::this_thread::get_id(); \
    }
        //d->thread.get_id() == std::thread::id{} || \

    StdThreadHandler::~StdThreadHandler()
    {
    }

    // async
    void StdThreadHandler::bindOuter(PrivateType self, PrivateType outer)
    {
        auto d = reinterpret_cast<StdThreadHandlerData*>(self->handlerData);

        d->sync.lock();
        d->defferedCalls.push_back(
            [self = self.data(), outer]()
            {
                assert(STH_THREAD_HANDLER_ASSERTION_CHECK());

                self->bindOuterLocal(outer);
            }
        );
        d->sync.unlock();
        d->notifier.notify_one();

        if(d->thread.get_id() == std::thread::id{})
            _executeOne(d);
    }

    // async
    void StdThreadHandler::fulfill(PrivateType self, std::any&& value, bool isResolve)
    {
        auto d = reinterpret_cast<StdThreadHandlerData*>(self->handlerData);

        d->sync.lock();
        d->defferedCalls.push_back(
            [self = self.data(), value = std::forward<std::any>(value), isResolve]() mutable
            {
                assert(STH_THREAD_HANDLER_ASSERTION_CHECK());

                self->fulfillLocal(std::move(value), isResolve ? CGull::Resolved : CGull::Rejected);
            }
        );
        d->sync.unlock();
        d->notifier.notify_one();

        if(d->thread.get_id() == std::thread::id{})
            _executeOne(d);
    }

    // async
    void StdThreadHandler::deleteThis(PrivateType self)
    {
        auto d = reinterpret_cast<StdThreadHandlerData*>(self->handlerData);

        d->sync.lock();
        d->defferedCalls.push_back(
            [self = self.data(), d]()
            {
                assert(STH_THREAD_HANDLER_ASSERTION_CHECK());

                self->deleteThisLocal();
            }
        );
        d->sync.unlock();
        d->notifier.notify_one();

        if(d->thread.get_id() == std::thread::id{})
            _executeOne(d);
    }

    // async
    void StdThreadHandler::tryFinish(PrivateType self)
    {
        auto d = reinterpret_cast<StdThreadHandlerData*>(self->handlerData);

        d->sync.lock();
        d->defferedCalls.push_back(
            [self = self.data()]()
            {
                assert(STH_THREAD_HANDLER_ASSERTION_CHECK());

                self->tryFinishLocal();
            }
        );
        d->sync.unlock();
        d->notifier.notify_one();

        if(d->thread.get_id() == std::thread::id{})
            _executeOne(d);
    }

    // local
    void StdThreadHandler::init(PrivateType self)
    {
        self->handlerData = reinterpret_cast<void*>(new StdThreadHandlerData);
    }

    // local
    void StdThreadHandler::bindInner(PrivateType self, PrivateType inner, CGull::WaitType waitType)
    {
        self->bindInnerLocal(inner, waitType);
    }

    // local
    void StdThreadHandler::setFinisher(PrivateType self, CallbackFunctor&& callback, bool isResolve)
    {
        auto d = reinterpret_cast<StdThreadHandlerData*>(self->handlerData);

        d->thread = std::thread{
            [self = self.data(), d]()
            {
                while(!d->exit)
                {
                    _executeOne(d);

                    if(self->_ref.load() == 1) // only this private remained
                    {
                        d->sync.lock();
                        const auto queueSize = d->defferedCalls.size();
                        d->sync.unlock();

                        if(queueSize == 0)
                        {
                            d->thread.detach();
                            self->deleteThisLocal();
                            return;
};
                    };
                };
            }
        };

#if defined(CGULL_DEBUG_GUTS)
        {
            std::string number(17, 0);
            std::snprintf((char*)number.c_str(), 17, "%llx", (intptr_t)self.constData());

            CGull::guts::_DebugSetThreadName(d->thread, "CGull::Promise{h:std}:" + number);
        }
#endif

        self->setFinisherLocal(std::forward<CallbackFunctor>(callback), isResolve);
    }

    // thread
    void StdThreadHandler::_executeOne(StdThreadHandlerData* d)
    {
        StdThreadHandler::CallType event;

        {
            std::unique_lock<std::mutex> lock{ d->sync };
            //d->notifier.wait(lock);

            if(d->defferedCalls.empty())
                return;

            event = std::move(d->defferedCalls.front());
            d->defferedCalls.pop_front();
        };

        event();
    }

    // thread
    void StdThreadHandler::abort(PrivateType self)
    {
        assert(STH_THREAD_HANDLER_ASSERTION_CHECK());

        self->abortLocal();
    }

#undef STH_HANDLER
#define STH_HANDLER d

    // thread
    void StdThreadHandler::deleteHandlerData(void* data)
    {
        auto d = reinterpret_cast<StdThreadHandlerData*>(data);

        if(d->thread.get_id() == std::thread::id{})
        {
            assert(STH_THREAD_HANDLER_ASSERTION_CHECK());

            d->exit = true;

            //! \todo no sync between thread::id and this code!!!

            if(!d->defferedCalls.empty())
                ; //! \todo abort

            delete d;
        }
        else
        {
            d->exit = true;

            if(d->thread.get_id() != std::this_thread::get_id())
                d->thread.join();
            else
                d->thread.detach();

            if(!d->defferedCalls.empty())
                ; //! \todo abort

            delete d;
        };
    }

    void StdThreadHandler::useForThisThread()
    {
        if(!_threadHandler())
            _threadHandler().reset(new StdThreadHandler());
    }

};
