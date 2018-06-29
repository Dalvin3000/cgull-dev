#pragma once

#include "handler.h"
#include "promise.h"

#include <thread>
#include <mutex>
#include <functional>
#include <deque>
#include <condition_variable>

namespace CGull
{

    class CGULL_API StdThreadHandler : public Handler
    {
    public:
        using CallType = std::function<void()>;
        using QueueType = std::deque< CallType >;


        ~StdThreadHandler() override;

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


    private:
        struct StdThreadHandlerData
        {
            static constexpr int QUEUE_MAX_SIZE = 1024;

            QueueType               defferedCalls;
            std::thread             thread;
            std::condition_variable notifier;
            std::mutex              sync;
            volatile bool           exit = false;

            StdThreadHandlerData()
                //: defferedCalls{ QUEUE_MAX_SIZE }
            { }
        };

        static void _executeOne(StdThreadHandlerData* d);
    };

};
