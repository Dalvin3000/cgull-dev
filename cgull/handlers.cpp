// Copyright 2018 Vladislav Yaremenko
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

#include "sync_handler.h"
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

    void SyncHandler::deleteHandlerData(PrivateType self)
    {
    }

    void SyncHandler::useForThisThread()
    {
        if(!_threadHandler())
            _threadHandler().reset(new SyncHandler());
    }

};
