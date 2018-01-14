

inline
Promise::Promise(const PrivateType& from)
    : _d(from)
{ }


inline
Promise::Promise()
    : _d(new CGull::guts::PromisePrivate{})
{ }


inline
Promise::~Promise()
{
    _abort();
}


template<
    typename _Resolve,
    typename _ResolveFunctor
>
Promise Promise::then(_Resolve&& onResolve)
{
    return _then(std::forward<_ResolveFunctor>(_ResolveFunctor{onResolve}), nullptr);
}


template <
    typename _Resolve,
    typename _Context
> inline
Promise Promise::_then(_Resolve&& onResolve, _Context context)
{
    (void)context; //! \todo implement contexts

    auto D = _d.data();

    // can't set finisher twice
    assert(D->state.load() >= CGull::NotFinished && D->state.load() <= CGull::RejectedFinished);

    // unnecessary check
    assert((bool)onResolve);

    // wrap finisher.
    // \param abort If true - then only captures will be cleared.
    D->finisher =
        [inner = *this, onResolve = std::move(onResolve)](int abort) mutable
        {
            if(!abort)
                inner._wrapRescue(onResolve);

            inner._d.reset();
        };

    // chain link
    Promise next;

    next._bindInner(*this, CGull::LastBound);

    // barrier & handle async ops
    D->state.store(CGull::AwaitingResolve);
    //D->handler->setFinisher();

    return next;
}


// ====  helpers  ====


template< typename _Callback > inline
void Promise::_wrapRescue(_Callback callback)
{
    try
    {
        _wrapCallbackReturn(callback);
    }
    catch(const std::exception& e)
    {
        _finishLocal(0, e.what());
    }
    catch(const char* e)
    {
        _finishLocal(0, e);
    }
    catch(...)
    {
        //! \todo exceptions path-through
        throw std::runtime_error("Promise::runFinisher: Uncaught exception inside callback (valid exception types: std::exception, const char*)");
    };
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

    _finishLocal(1, std::any{});
}


//! \note lambda [](...) -> std::any
template< typename _Callback > inline
void Promise::_wrapCallbackReturn(_Callback& callback, CGull::guts::return_any_tag)
{
    _finishLocal(1, std::move(_wrapCallbackArgs(callback)));
}


//! \note lambda [](...) -> Promise
template< typename _Callback > inline
void Promise::_wrapCallbackReturn(_Callback& callback, CGull::guts::return_promise_tag)
{
    Promise inner = _wrapCallbackArgs(callback);

    _bindInner(inner, CGull::LastBound);
}


//! \note lambda [](...) -> auto
template< typename _Callback > inline
void Promise::_wrapCallbackReturn(_Callback& callback, CGull::guts::return_auto_tag)
{
    _finishLocal(1, std::make_any(_wrapCallbackArgs(callback)));
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
    return callback(_valueLocal());
}


//! \note lambda [](auto) -> auto
template<
    typename _Callback,
    typename _CallbackTraits
> inline
auto Promise::_wrapCallbackArgs(_Callback& callback, CGull::guts::args_count_1_auto)
    -> typename _CallbackTraits::result_type
{
    return callback(_unwrapCallbackArg< typename _CallbackTraits::template arg<0>::type >(_valueLocal()));
}


template< typename _T >
const _T& Promise::_unwrapCallbackArg(std::any& value)
{
    try
    {
        return std::any_cast< const _T& >(value);
    }
    catch(const std::bad_any_cast&)
    {
        static const std::any default_value;
        return std::any_cast< const _T& >(default_value);
    };
}
