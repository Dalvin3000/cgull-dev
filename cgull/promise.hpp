// Copyright 2018 Vladislav Yaremenko
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.




// ====  private  ====


inline
Promise::Promise(const PrivateType& from)
    : _d(from)
{ }


template <
    typename _Resolve,
    typename _Context
> inline
Promise Promise::_then(_Resolve&& onResolve, _Context context, bool isResolve)
{
    (void)context; //!< \todo implement contexts

    auto D = _d.data();

    // probably unnecessary check
    assert((bool)onResolve);

    // chained outer
    Promise next;

    // Wrap finisher into unified functor.
    // \param abort If true then only captures will be cleared.
    // \param innersResult Cached result of inner promises.
    CGull::CallbackFunctor wrappedFinisher =
        [_self = next, onResolve = std::move(onResolve)](bool abort, std::any&& innersResult) mutable
        {
            auto self = _self;
            _self._d.reset();

            if(!self._d)
                return;

            if(!abort)
            {
                self._d->innersResultCache = std::move(innersResult);

                self._wrapRescue(std::move(onResolve));
            }
            else
                const auto temp = std::move(onResolve);
        };

    // we don't need to call handler cause 'next' was just created
    next._d->setFinisherLocal(std::move(wrappedFinisher), isResolve);
    next._d->bindInnerLocal(_d, CGull::LastBound);

    // async bind 'next' as 'outer' to 'this' cause this thread might not be the thread of 'this'.
    _handleBindOuter(next);

    return next;
}


inline
void Promise::_handleFulfilled()
{
    _d->handler->tryFinish(_d);
}


inline
void Promise::_handleBindInner(Promise inner, CGull::WaitType waitType)
{
    _d->handler->bindInner(_d, inner._d, waitType);
}


inline
void Promise::_handleBindOuter(Promise outer)
{
    _d->handler->bindOuter(_d, outer._d);
}


inline
void Promise::_handleAbort()
{
    _d->handler->abort(_d);
}


inline
void Promise::_handleDeleteThis()
{
    auto D = _d.data();

    if(D)
        D->handler->deleteThis(_d);
}


inline
void Promise::_handleSetFinisher(CGull::CallbackFunctor&& callback, bool isResolve)
{
    _d->handler->setFinisher(_d, std::forward<decltype(callback)>(callback), isResolve);
}


template< > inline
void Promise::_handleFulfill(std::any&& value, bool isResolve)
{
    _d->handler->fulfill(_d, std::forward<std::any>(value), isResolve);
}


template< typename _T > inline
void Promise::_handleFulfill(_T&& value, bool isResolve)
{
    _d->handler->fulfill(_d, std::make_any<_T>(std::forward<_T>(value)), isResolve);
}


template< > inline
void Promise::_handleFulfill(const std::any& value, bool isResolve)
{
    _d->handler->fulfill(_d, std::any{value}, isResolve);
}


template< typename _T > inline
void Promise::_handleFulfill(const _T& value, bool isResolve)
{
    _d->handler->fulfill(_d, std::make_any<_T>(value), isResolve);
}



// ====  public  ====


inline
Promise::Promise()
    : _d(new CGull::guts::PromisePrivate{})
{
    _d->handler = CGull::Handler::forThisThread(); //!< \todo implement contexts
}


inline
Promise::~Promise()
{
    _handleDeleteThis();
}


template<
    typename _Resolve,
    typename _ResolveFunctor
>
Promise Promise::then(_Resolve&& onResolve)
{
    return _then(std::forward<_ResolveFunctor>(_ResolveFunctor{onResolve}), nullptr, true);
}


template<
    typename _Reject,
    typename _RejectFunctor
>
Promise Promise::rescue(_Reject&& onReject)
{
    return _then(std::forward<_RejectFunctor>(_RejectFunctor{ onReject }), nullptr, false);
}


inline
Promise& Promise::resolve(const std::any& value)
{
    _handleFulfill(value, true);

    return *this;
}


inline
Promise& Promise::resolve(std::any&& value)
{
    _handleFulfill(std::forward<std::any>(value), true);

    return *this;
}


inline
Promise& Promise::reject(const std::any& value)
{
    _handleFulfill(value, false);

    return *this;
}


inline
Promise& Promise::reject(std::any&& value)
{
    _handleFulfill(std::forward<std::any>(value), false);

    return *this;
}



// ====  helpers  ====


template< typename _Callback > inline
void Promise::_wrapRescue(_Callback&& callback)
{
    try
    {
        _wrapRescue(callback, typename CGull::guts::function_traits<_Callback>::result_tag{});
    }
    catch(const std::exception& e)
    {
        _d->finishState = CGull::Skipped;
        _d->fulfillLocal(e.what(), CGull::Rejected);
    }
    catch(const char* e)
    {
        _d->finishState = CGull::Skipped;
        _d->fulfillLocal(e, CGull::Rejected);
    }
    catch(std::any e)
    {
        _d->finishState = CGull::Skipped;
        _d->fulfillLocal(std::move(e), CGull::Rejected);
    }
    catch(...)
    {
        //! \todo exceptions path-through
        throw std::runtime_error(
            "Promise::runFinisher: Uncaught exception inside callback "
            "(valid exception types: std::exception, const char*, std::any, CGull::guts::functor_t<>::result_type)"
        );
    };
}


//! \note lambda [](...) -> auto
template< typename _Callback > inline
void Promise::_wrapRescue(_Callback&& callback, CGull::guts::return_auto_tag)
{
    try
    {
        _wrapCallbackReturn(callback);
    }
    catch(typename CGull::guts::functor_t<_Callback>::result_type& e)
    {
        _d->finishState = CGull::Skipped;
        _d->fulfillLocal(std::any{e}, CGull::Rejected);
    };
}


//! \note lambda [](...) -> Promise
template< typename _Callback > inline
void Promise::_wrapRescue(_Callback&& callback, CGull::guts::return_promise_tag)
{
    try
    {
        _wrapCallbackReturn(callback);
    }
    catch(Promise e)
    {
        _d->finishState = CGull::Skipped;
        _d->fulfillLocal(std::any{e}, CGull::Rejected);
    };
}


//! \note lambda [](...) -> std::any
template< typename _Callback > inline
void Promise::_wrapRescue(_Callback&& callback, CGull::guts::return_any_tag)
{
    try
    {
        _wrapCallbackReturn(callback);
    }
    catch(std::any& e)
    {
        _d->finishState = CGull::Skipped;
        _d->fulfillLocal(std::move(e), CGull::Rejected);
    };
}


//! \note lambda [](...) -> void
template< typename _Callback > inline
void Promise::_wrapRescue(_Callback&& callback, CGull::guts::return_void_tag)
{
    _wrapCallbackReturn(callback);
}


template< typename _Callback > inline
void Promise::_wrapCallbackReturn(_Callback& callback)
{
    _wrapCallbackReturn(callback, typename CGull::guts::function_traits<_Callback>::result_tag{});
}


//! \note lambda [](...) -> void
template< typename _Callback > inline
void Promise::_wrapCallbackReturn(_Callback& callback, CGull::guts::return_void_tag)
{
    _wrapCallbackArgs(callback);

    _d->finishState = _d->finisherIsResolver ? CGull::Thenned : CGull::Rescued;
    _d->fulfillLocal(std::move(std::any{}), CGull::Resolved);
}


//! \note lambda [](...) -> std::any
template< typename _Callback > inline
void Promise::_wrapCallbackReturn(_Callback& callback, CGull::guts::return_any_tag)
{
    std::any result = _wrapCallbackArgs(callback);

    _d->completeFinish();
    _d->fulfillLocal(std::move(result), CGull::Resolved);
}


//! \note lambda [](...) -> auto
template< typename _Callback > inline
void Promise::_wrapCallbackReturn(_Callback& callback, CGull::guts::return_auto_tag)
{
    auto result = _wrapCallbackArgs(callback);

    _d->completeFinish();
    _d->fulfillLocal(std::make_any<decltype(result)>(result), CGull::Resolved);
}


//! \note lambda [](...) -> Promise
template< typename _Callback > inline
void Promise::_wrapCallbackReturn(_Callback& callback, CGull::guts::return_promise_tag)
{
    Promise inner = _wrapCallbackArgs(callback);

    _d->completeFinish();
    _d->bindInnerLocal(inner._d, CGull::LastBound);
    inner._handleBindOuter(_d);
}


//! Wraps finisher callback's argumets.
template<
    typename _Callback,
    typename _CallbackTraits
> inline
auto Promise::_wrapCallbackArgs(_Callback& callback)
    -> typename _CallbackTraits::result_type
{
    return _wrapCallbackArgs(callback, typename CGull::guts::function_traits<_Callback>::args_tag{});
}


//! \note lambda [](void) -> auto
template<
    typename _Callback,
    typename _CallbackTraits
> inline
auto Promise::_wrapCallbackArgs(_Callback& callback, CGull::guts::args_count_0)
    -> typename _CallbackTraits::result_type
{
    return callback();
}


//! \note lambda [](std::any) -> auto
template<
    typename _Callback,
    typename _CallbackTraits
> inline
auto Promise::_wrapCallbackArgs(_Callback& callback, CGull::guts::args_count_1_any)
    -> typename _CallbackTraits::result_type
{
    return callback(std::move(_d->innersResultCache));
}


//! \note lambda [](auto) -> auto
template<
    typename _Callback,
    typename _CallbackTraits
> inline
auto Promise::_wrapCallbackArgs(_Callback& callback, CGull::guts::args_count_1_auto)
    -> typename _CallbackTraits::result_type
{
    return callback(std::move(
        _unwrapArg< typename _CallbackTraits::template arg<0>::type >
        (std::move(_d->innersResultCache)))
    );
}


template< typename _T, typename _dT > inline
_dT Promise::_unwrapArg(std::any&& value)
{
    try
    {
        return std::any_cast< _dT >(value);
    }
    catch(const std::bad_any_cast&)
    {
        return _dT{};
    };
}
