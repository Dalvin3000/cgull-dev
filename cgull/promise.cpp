// Copyright 2018 Vladislav Yaremenko
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

#include "promise.h"

#if defined(CGULL_DEBUG_GUTS)
#   ifdef _WIN32
#       define WIN32_LEAN_AND_MEAN
#       include <windows.h>
#   else
#       include <sys/prctl.h>
#   endif
#endif


namespace CGull::guts
{
#if defined(CGULL_DEBUG_GUTS)

    std::atomic_int& _debugPrivateCounter()
    {
        static std::atomic_int v = 0;

        return v;
    };

    std::mutex& _debugCoutMutex()
    {
        static std::mutex v;
        return v;
    }

    _DebugPromiseList& _DebugPromiseList::instance()
    {
        static _DebugPromiseList v{new std::deque<PromisePrivate*>{}, new std::mutex{}};
        return v;
    }

    void _DebugPromiseList::add(PromisePrivate* pPrivate)
    {
        std::unique_lock<std::mutex> lock{ *_guard };

        const auto i = std::find(_promises->cbegin(), _promises->cend(), pPrivate);

        if(i == _promises->cend())
            _promises->push_back(pPrivate);
    }

    void _DebugPromiseList::remove(PromisePrivate* pPrivate)
    {
        std::unique_lock<std::mutex> lock{ *_guard };

        const auto i = std::find(_promises->cbegin(), _promises->cend(), pPrivate);

        if(i != _promises->cend())
            _promises->erase(i);
    }

#ifdef _WIN32
    void _DebugSetThreadName(std::thread& thread, const std::string& name)
    {
#pragma pack(push,8)
        struct THREADNAME_INFO
        {
            DWORD  dwType; // Must be 0x1000.
            LPCSTR szName; // Pointer to name (in user addr space).
            DWORD  dwThreadID; // Thread ID (-1=caller thread).
            DWORD  dwFlags; // Reserved for future use, must be zero.
        };
#pragma pack(pop)

        const THREADNAME_INFO info =
            {
                0x1000,
                name.c_str(),
                ::GetThreadId(static_cast<HANDLE>(thread.native_handle())),
                0
            };

        __try
        {
            RaiseException(0x406D1388, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        { }
    }

#else
    void _DebugSetThreadName(std::thread& thread, const std::string& name)
    {
        pthread_setname_np(thread.native_handle(), name.c_str());
    }

#endif

#endif
};
