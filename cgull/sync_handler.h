// Copyright 2018 Vladislav Yaremenko
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

#pragma once

#include "handler.h"
#include "promise.h"

namespace CGull
{

    class CGULL_API SyncHandler : public Handler
    {
    public:
        ~SyncHandler() override;

        void setFinisher(PrivateType self, CallbackFunctor&& callback, bool isResolve) override;
        void bindInner(PrivateType self, PrivateType inner, CGull::WaitType waitType) override;
        void bindOuter(PrivateType self, PrivateType outer) override;
        void tryFinish(PrivateType self) override;
        void abort(PrivateType self) override;
        void fulfill(PrivateType self, std::any&& value, bool isResolve) override;
        void deleteThis(PrivateType self) override;
        void init(PrivateType self) override;
        void deleteHandlerData(void* data) override;

        static void useForThisThread();

    };

};
