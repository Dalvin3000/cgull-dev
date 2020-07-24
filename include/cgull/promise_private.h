#pragma once

#include "config.h"
#include "common.h"
#include "guts/shared_data.h"

#include <assert.h>
#include <vector>
#include <any>
#include <tuple>
#include <stdexcept>


CGULL_NAMESPACE_START


class handler;


class promise_private : public guts::shared_data
{
    CGULL_DISABLE_COPY(promise_private);
    CGULL_DISABLE_MOVE(promise_private);

    friend class promise;

public:
    using type = guts::shared_data_ptr<promise_private>;

    using promise_private_list = std::vector<type>;

    using finisher_type = std::function<finisher_t>;


    promise_private() = default;
    promise_private(wait_t wait);

    void local_fulfill(std::any&& value, fulfillment_state_t state) noexcept;
    void local_set_finisher(finisher_type&& callback, bool is_resolver) noexcept;
    void local_try_finish() noexcept;
    void local_abort() noexcept;
    void local_bind_inner(type inner, wait_t new_wait_type) noexcept;
    void local_bind_outer(type outer) noexcept;

    [[nodiscard]]
    fulfillment_state_t fulfillment() const noexcept;
    [[nodiscard]]
    bool                is_fulfilled() const noexcept;
    [[nodiscard]]
    finish_state_t      finish() const noexcept;
    [[nodiscard]]
    wait_t              wait() const noexcept;


protected:
    //! \note Context-local.
    std::any                    result;

    //! \note Async.
    atomic_fulfillment_state    fulfillment_state = not_fulfilled;
    //! \note Async.
    //! \todo local?
    finish_state_t              finish_state = not_finished;
    //! \note Context-local.
    wait_t                      wait_type = any;

    //! Context-local.
//     std::any                    inners_result_cache;
    //! Context-local.
    promise_private_list        inners;
    //! Context-local.
    promise_private_list        outers;

    //! \note Context-local.
    finisher_type               finisher;
    //! \note Context-local.
    bool                        resolve_finisher;

    //! Async operations handler.
    //! \note If not set, then promise will be context-local.
    //! \note Context-local.
    handler*                    handler = nullptr;


private:
    std::tuple<fulfillment_state_t, std::any> _check_inners_fulfillment() noexcept;
    //! Called only when finished and fulfilled.
    void _propagate() noexcept;
    void _unbind_inners() noexcept;
    void _unbind_outers() noexcept;

};


CGULL_NAMESPACE_END


#include "handler.h"


CGULL_NAMESPACE_START


inline
void promise_private::local_fulfill(std::any&& value, fulfillment_state_t state) noexcept
{
    auto nff = not_fulfilled;

    if(!fulfillment_state.compare_exchange_strong(
        nff, fulfilling_now,
        std::memory_order::acquire, std::memory_order::relaxed
    ))
        return;

    result = std::forward<decltype(value)>(value);

    fulfillment_state.store(state, std::memory_order::release);
}


inline
void promise_private::local_set_finisher(finisher_type&& callback, bool is_resolver) noexcept
{
    const auto fn_st = finish();

    assert(!fn_st && "Promise already finished.");
    assert(!finisher && "Can't set finisher twice.");

    if(fn_st)
        return;

    finisher = std::forward<decltype(callback)>(callback);
    resolve_finisher = is_resolver;
    finish_state = awaiting;

    // call finisher if promise already fulfilled
    if(fulfillment())
        local_try_finish();
}


inline
void promise_private::local_try_finish() noexcept
{
    const auto fn_state = finish();

    // promise already finished
    if(fn_state >= thenned && is_fulfilled())
        return;

    // check inners for outer promise or just return if promise not chained
    if(inners.empty())
        return;

    auto [ins_state, ins_result] = _check_inners_fulfillment();

    // inners not ready
    if(ins_state < resolved)
        return;

    // finish or try propagate to outer. skip if chain is also skipped.
    if( fn_state == awaiting && (ins_state == resolved || ins_state == rejected) )
    {
        assert(!!finisher && "cgull:try_finish() can't be called without callback set.");

        finisher(execute, std::move(ins_result)); // not async
        finisher = nullptr;
    }
    else
    {
        _unbind_inners();

        local_fulfill(std::move(ins_result), ins_state);
        local_abort();
    };
}


inline
void promise_private::local_abort() noexcept
{
    if(finisher)
    {
        finisher(abort, std::any{});
        finisher = nullptr;
    };

    if(finish() < thenned)
        finish_state = skipped;

    _unbind_inners();
    _propagate();
}


inline
void promise_private::local_bind_inner(type inner, wait_t new_wait_type) noexcept
{
    inners.push_back(inner);
    wait_type = new_wait_type;
}


inline
void promise_private::local_bind_outer(type outer) noexcept
{

    if(is_fulfilled())
        outer->handler->try_finish();
    else
        outers.push_back(outer);
}


inline
fulfillment_state_t promise_private::fulfillment() const noexcept
{
    return fulfillment_state;
}


inline
bool promise_private::is_fulfilled() const noexcept
{
    return fulfillment() >= resolved;
}


inline
finish_state_t promise_private::finish() const noexcept
{
    return finish_state;
}


inline
wait_t promise_private::wait() const noexcept
{
    return wait_type;
}


inline
std::tuple<fulfillment_state_t, std::any> promise_private::_check_inners_fulfillment() noexcept
{
    const auto wt = wait();
    const auto ins_count = inners.size();

    switch(wt)
    {
    default:
        assert(false && "cgull: wait type unknown or not implemented");
    case any:
    {
        bool something_still_unfinished = false;

        for(const auto& inn : inners)
        {
            if(!inn->is_fulfilled())                // don't need to check finish at this moment
                something_still_unfinished = true;
            else if(inn->fulfillment() == resolved) // resolved inner found
                return { resolved, inn->result };
        };

        // still have not finished and fulfilled inners
        if(something_still_unfinished)
            return { not_fulfilled, std::any{} };

        // reject on no resolved promise
        any_list results;

        results.reserve(ins_count);

        for(const auto& inn : inners)
            results.emplace_back(inn->result);

        return { rejected, std::any{std::move(results)} };
    }
    case all:
    {
        for(const auto& inn : inners)
        {
            if(inn->fulfillment() == rejected)  // one of inners rejected
                return { rejected, inn->result };
            else if(!inn->is_fulfilled())       // still have not finished
                return { not_fulfilled, std::any{} };
        };

        // resolve on all resolved promises
        any_list results;

        results.reserve(ins_count);

        for(const auto& inn : inners)
            results.emplace_back(inn->result);

        return { resolved, std::any{std::move(results)} };
    }
    case first:
    {
        for(const auto& inn : inners)
        {
            if(inn->is_fulfilled())
                return { inn->fulfillment(), inn->result };
        };

        return { not_fulfilled, std::any{} };
    }
    case last_bound:
    {
        const auto inn = inners.back();

        if(!inn->is_fulfilled())
            return { not_fulfilled, std::any{} };

        return { inn->fulfillment(), inn->result };
    }
    };

    return { not_fulfilled, std::any{} };
}


inline
void promise_private::_propagate() noexcept
{
    for(const auto& outer : outers)
    {
        if(outer->handler)
            outer->handler->try_finish();
        else
            outer->local_try_finish();
    };

    _unbind_outers();
}


inline
void promise_private::_unbind_inners() noexcept
{
    if(!inners.empty())
        inners.clear();
}


inline
void promise_private::_unbind_outers() noexcept
{
    if(!outers.empty())
        outers.clear();
}



CGULL_NAMESPACE_END
