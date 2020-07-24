


// ====  private  ====


inline
promise::promise(const PrivateType& from)
    : _d(from)
{ }


template <
    typename _Resolve,
    typename _Context
> inline
promise promise::_then(_Resolve&& onResolve, _Context context, bool isResolve)
{
    (void)context; //!< \todo implement contexts

//     auto D = _d.data();

    // chained outer
    promise next;

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
    next._d->handler->setFinisher(next._d, std::move(wrappedFinisher), isResolve);
    next._d->bindInnerLocal(_d, CGull::LastBound);

    // async bind 'next' as 'outer' to 'this' cause this thread might not be the thread of 'this'.
    _handleBindOuter(next._d);

    return next;
}


inline
void promise::_handleBindOuter(PrivateType outer)
{
    _d->handler->bindOuter(_d, outer);
}


template< > inline
void promise::_handleFulfill(std::any&& value, bool isResolve)
{
    _d->handler->fulfill(_d, std::forward<std::any>(value), isResolve);
}


template< typename _T, typename > inline
void promise::_handleFulfill(_T value, bool isResolve)
{
    _d->handler->fulfill(_d, std::make_any<_T>(std::forward<_T>(value)), isResolve);
}


template< > inline
void promise::_handleFulfill(const std::any& value, bool isResolve)
{
    _d->handler->fulfill(_d, std::any{value}, isResolve);
}


template< typename _T > inline
void promise::_handleFulfill(const _T& value, bool isResolve)
{
    _d->handler->fulfill(_d, std::make_any<_T>(value), isResolve);
}



// ====  public  ====


inline
promise::promise()
    : _d(new CGull::guts::PromisePrivate{})
{
    _d->handler = CGull::Handler::forThisThread(); //!< \todo implement contexts
    _d->handler->init(_d);
}


inline
promise::~promise()
{
    auto D = _d.data();

    if(D)
        D->handler->deleteThis(_d);
}


template<
    typename _Resolve,
    typename _ResolveFunctor
>
promise promise::then(_Resolve&& onResolve)
{
    return _then(std::forward<_ResolveFunctor>(_ResolveFunctor{onResolve}), nullptr, true);
}


template<
    typename _Reject,
    typename _RejectFunctor
>
promise promise::rescue(_Reject&& onReject)
{
    return _then(std::forward<_RejectFunctor>(_RejectFunctor{ onReject }), nullptr, false);
}


inline
promise& promise::resolve(const std::any& value)
{
    _handleFulfill(value, true);

    return *this;
}


inline
promise& promise::resolve(std::any&& value)
{
    _handleFulfill(std::forward<std::any>(value), true);

    return *this;
}


inline
promise& promise::reject(const std::any& value)
{
    _handleFulfill(value, false);

    return *this;
}


inline
promise& promise::reject(std::any&& value)
{
    _handleFulfill(std::forward<std::any>(value), false);

    return *this;
}



// ====  helpers  ====


template< typename _Callback > inline
void promise::_wrapRescue(_Callback&& callback)
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
            "promise::runFinisher: Uncaught exception inside callback "
            "(valid exception types: std::exception, const char*, std::any, CGull::guts::functor_t<>::result_type)"
        );
    };
}


//! \note lambda [](...) -> auto
template< typename _Callback > inline
void promise::_wrapRescue(_Callback&& callback, CGull::guts::return_auto_tag)
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


//! \note lambda [](...) -> promise
template< typename _Callback > inline
void promise::_wrapRescue(_Callback&& callback, CGull::guts::return_promise_tag)
{
    try
    {
        _wrapCallbackReturn(callback);
    }
    catch(promise e)
    {
        _d->finishState = CGull::Skipped;
        _d->fulfillLocal(std::any{e}, CGull::Rejected);
    };
}


//! \note lambda [](...) -> std::any
template< typename _Callback > inline
void promise::_wrapRescue(_Callback&& callback, CGull::guts::return_any_tag)
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
void promise::_wrapRescue(_Callback&& callback, CGull::guts::return_void_tag)
{
    _wrapCallbackReturn(callback);
}


template< typename _Callback > inline
void promise::_wrapCallbackReturn(_Callback& callback)
{
    _wrapCallbackReturn(callback, typename CGull::guts::function_traits<_Callback>::result_tag{});
}


//! \note lambda [](...) -> void
template< typename _Callback > inline
void promise::_wrapCallbackReturn(_Callback& callback, CGull::guts::return_void_tag)
{
    _wrapCallbackArgs(callback);

    _d->finishState = _d->finisherIsResolver ? CGull::Thenned : CGull::Rescued;
    _d->fulfillLocal(std::move(std::any{}), CGull::Resolved);
}


//! \note lambda [](...) -> std::any
template< typename _Callback > inline
void promise::_wrapCallbackReturn(_Callback& callback, CGull::guts::return_any_tag)
{
    std::any result = _wrapCallbackArgs(callback);

    _d->completeFinish();
    _d->fulfillLocal(std::move(result), CGull::Resolved);
}


//! \note lambda [](...) -> auto
template< typename _Callback > inline
void promise::_wrapCallbackReturn(_Callback& callback, CGull::guts::return_auto_tag)
{
    auto result = _wrapCallbackArgs(callback);

    _d->completeFinish();
    _d->fulfillLocal(std::make_any<decltype(result)>(result), CGull::Resolved);
}


//! \note lambda [](...) -> promise
template< typename _Callback > inline
void promise::_wrapCallbackReturn(_Callback& callback, CGull::guts::return_promise_tag)
{
    promise inner = _wrapCallbackArgs(callback);

    _d->completeFinish();
    _d->bindInnerLocal(inner._d, CGull::LastBound);
    inner._handleBindOuter(_d);
}


//! Wraps finisher callback's argumets.
template<
    typename _Callback,
    typename _CallbackTraits
> inline
auto promise::_wrapCallbackArgs(_Callback& callback)
    -> typename _CallbackTraits::result_type
{
    return _wrapCallbackArgs(callback, typename CGull::guts::function_traits<_Callback>::args_tag{});
}


//! \note lambda [](void) -> auto
template<
    typename _Callback,
    typename _CallbackTraits
> inline
auto promise::_wrapCallbackArgs(_Callback& callback, CGull::guts::args_count_0)
    -> typename _CallbackTraits::result_type
{
    return callback();
}


//! \note lambda [](std::any) -> auto
template<
    typename _Callback,
    typename _CallbackTraits
> inline
auto promise::_wrapCallbackArgs(_Callback& callback, CGull::guts::args_count_1_any)
    -> typename _CallbackTraits::result_type
{
    return callback(std::move(_d->innersResultCache));
}


//! \note lambda [](auto) -> auto
template<
    typename _Callback,
    typename _CallbackTraits
> inline
auto promise::_wrapCallbackArgs(_Callback& callback, CGull::guts::args_count_1_auto)
    -> typename _CallbackTraits::result_type
{
    return callback(std::move(
        _unwrapArg< typename _CallbackTraits::template arg<0>::type >
        (std::move(_d->innersResultCache)))
    );
}


template< typename _T, typename _dT > inline
_dT promise::_unwrapArg(std::any&& value)
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
