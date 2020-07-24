#pragma once

#include "config.h"

#include <stdint.h>
#include <atomic>
#include <functional>
#include <any>
#include <vector>


CGULL_NAMESPACE_START


//! Represents promise fulfillment.
enum fulfillment_state_t : int8_t
{
    not_fulfilled = 0,
    fulfilling_now,
    resolved,
    rejected,
    aborted,
};

//! Represents promise finish state.
enum finish_state_t : int8_t
{
    not_finished = 0,
    awaiting,
    thenned,
    rescued,
    skipped,
};

//! Represents promise wait type for nested promises.
enum wait_t : int8_t
{
    any = 0,
    all,
    first,
    last_bound,
};


using atomic_fulfillment_state = std::atomic<fulfillment_state_t>;
using atomic_finish_state = std::atomic<finish_state_t>;


//! Finisher return sugar.
inline constexpr const bool abort = true;
//! Finisher return sugar.
inline constexpr const bool execute = false;


using any_list = std::vector<std::any>;


using finisher_t = void(bool is_abort, std::any&& inners_result);


CGULL_NAMESPACE_END
