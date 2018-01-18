#include "sync_handler.h"
#include "promise.h"


namespace CGull
{

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

    void SyncHandler::fulfilled(PrivateType self)
    {
        self->checkFulfillmentLocal();
    }

    void SyncHandler::abort(PrivateType self)
    {
        self->abortLocal();
    }

    void SyncHandler::deleteThis(PrivateType self)
    {
        //! \todo fix this
    }

    void SyncHandler::useForThisThread()
    {
        if(!_threadHandler())
            _threadHandler().reset(new SyncHandler());
    }

};
