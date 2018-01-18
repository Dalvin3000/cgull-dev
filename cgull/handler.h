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
        virtual void fulfilled(PrivateType self) = 0;
        virtual void abort(PrivateType self) = 0;
        virtual void deleteThis(PrivateType self) = 0;

        //! \return handler for current thread.
        static Handler* forThisThread();

    protected:
        static std::unique_ptr<Handler>& _threadHandler();

    };

};


#include "promise_private.hpp"
