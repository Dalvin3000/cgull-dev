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
        void fulfilled(PrivateType self) override;
        void abort(PrivateType self) override;
        void deleteThis(PrivateType self) override;

        static void useForThisThread();


    private:

    };

};
