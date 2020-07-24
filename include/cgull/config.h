#pragma once

#include "platform_detection.h"
#include "cgull_export.h"


#define CGULL_DEBUG_GUTS


#ifndef CGULL_NAMESPACE_START
#   define CGULL_NAMESPACE cgull
#   define CGULL_NAMESPACE_START namespace CGULL_NAMESPACE {
#   define CGULL_NAMESPACE_END };
#endif

#ifndef CGULL_GUTS_NAMESPACE_START
#   define CGULL_GUTS_NAMESPACE guts
#   define CGULL_GUTS_NAMESPACE_START namespace CGULL_GUTS_NAMESPACE {
#   define CGULL_GUTS_NAMESPACE_END };
#endif


#define CGULL_DISABLE_COPY(c)              \
    private:                               \
        c(const c&) = delete;              \
        c& operator=(const c&) = delete;


#define CGULL_DISABLE_MOVE(c)              \
    private:                               \
        c(c&&) = delete;                   \
        c& operator=(c&&) = delete;

#define _cgull_context_local_s if(!d->handler)
#define _cgull_context_local _cgull_context_local_s d
#define _cgull_async if(d->handler) d->handler

#define _cgull_d auto d = _d.data();
