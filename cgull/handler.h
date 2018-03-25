// Copyright 2018 Vladislav Yaremenko
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

#pragma once

#include "promise_private.h"

#include <memory>

#define CGULL_DEFAULT_HANDLER SyncHandler

#if defined(CGULL_USE_NAMESPACE)
namespace CGull
{
#endif

    class Promise;

#if defined(CGULL_USE_NAMESPACE)
};
#endif


namespace CGull
{
    namespace guts
    {
        class PromisePrivate;
    };


    class CGULL_API Handler
    {
    public:
        using PrivateType = CGull::guts::PromisePrivate::Type;

        virtual ~Handler() = default;

        virtual void setFinisher(PrivateType self, CallbackFunctor&& callback, bool isResolve) = 0;
        virtual void bindInner(PrivateType self, PrivateType inner, CGull::WaitType) = 0;
        virtual void bindOuter(PrivateType self, PrivateType outer) = 0;
        virtual void tryFinish(PrivateType self) = 0;
        virtual void abort(PrivateType self) = 0;
        virtual void fulfill(PrivateType self, std::any&& value, bool isResolve) = 0;
        virtual void deleteThis(PrivateType self) = 0;

        //! \return handler for current thread.
        static Handler* forThisThread();

    protected:
        static std::unique_ptr<Handler>& _threadHandler();

    };

};


#include "promise_private.hpp"
