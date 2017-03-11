
#ifdef PLATFORM_WIN32
#include <windows.h>
#elif defined(PLATFORM_IOS) || defined(PLATFORM_LINUX)
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#endif

#include "../include/ThreadPool.h"
#include "AXP/cplusplus/xplatform/include/atime.h"

using namespace AXP;
using namespace IPC;

#ifdef PLATFORM_WIN32
#define PFUNC LPTHREAD_START_ROUTINE
#elif defined(PLATFORM_IOS) || defined(PLATFORM_LINUX)
typedef void *(*PFUNC)(void*);
#endif

Sp<ThreadPool::ThreadEx> ThreadPool::ThreadEx::Create(
    IN CONST Sp<ThreadPool> & pool)
{
    Sp<ThreadEx> object = new ThreadEx();
    if (!object)
        return NULL;

    if (AFAILED(object->Initialize(pool))) {
        return NULL;
    }

    return object;
}

ThreadPool::ThreadEx::ThreadEx()
{
#ifdef PLATFORM_WIN32
    mHandle = NULL;
    mId = 0;
#elif defined(PLATFORM_IOS) || defined(PLATFORM_LINUX)
    mThreadExists = FALSE;
#endif
    mThreadPool = NULL;
    mRoutine = NULL;
    mParam = NULL;
    mRunFlag = TRUE;
    mWait = TRUE;
    mEvent = NULL;
}

ThreadPool::ThreadEx::~ThreadEx()
{
#ifdef PLATFORM_WIN32
    if (mHandle)
        ::CloseHandle(mHandle);
#endif
}

ARESULT ThreadPool::ThreadEx::Initialize(
    IN CONST Sp<ThreadPool> & pool)
{
    mThreadPool = pool;

    mEvent = Event::Create();
    if (!mEvent)
        return AE_OUTOFMEMORY;

#ifdef PLATFORM_WIN32
    mHandle = ::CreateThread(NULL, 0, Routine, this, 0, &mId);
    if (!mHandle) {
        mEvent = NULL;
        return AE_OUTOFMEMORY;
    }
#elif defined(PLATFORM_IOS) || defined(PLATFORM_LINUX)
    if (::pthread_create(&mHandle, NULL, (PFUNC)Routine, this) != 0)
        return AE_FAIL;

    mThreadExists = TRUE;
#endif

    return AS_OK;
}

Void ThreadPool::ThreadEx::SetRoutine(
    IN PVoid routine,
    IN PVoid param)
{
    Synchronized(&mLock) {
        mRoutine = routine;
        mParam = param;
    }
}

ARESULT ThreadPool::ThreadEx::StartRoutine()
{
    mWait = FALSE;
    mEvent->Notify();

    return AS_OK;
}

ARESULT ThreadPool::ThreadEx::Discard()
{
    mRunFlag = FALSE;
    mEvent->Notify();

    return AS_OK;
}

ARESULT ThreadPool::ThreadEx::Join()
{
#ifdef PLATFORM_WIN32
    if (mHandle) {
        ::WaitForSingleObject(mHandle, INFINITE);
        mId = 0;
        mHandle = NULL;
    }
#elif defined(PLATFORM_IOS) || defined(PLATFORM_LINUX)
    if (mThreadExists) {
        pthread_join(mHandle, NULL);
        mThreadExists = FALSE;
    }
#endif // PLATFORM_WIN32

    return AS_OK;
}

ULong STDCALL ThreadPool::ThreadEx::Routine(
    IN PVoid param)
{
#if defined(PLATFORM_IOS) || defined(PLATFORM_LINUX)
    sigset_t signal_mask;
    sigemptyset (&signal_mask);
    sigaddset (&signal_mask, SIGPIPE);
    Int32 rc = pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);
    if (rc != 0)
        return -1;
#endif
    
    Sp<ThreadEx> pThis = (ThreadEx*)param;
    PFUNC routine = NULL;
    PVoid threadParam = NULL;

    while (pThis->mRunFlag) {
        WaitResult result = pThis->mEvent->Join();
        if (result != WaitResult_OK) {
            AXP::Sleep(100);
            continue;
        }

        if (!pThis->mRunFlag)
            break;

        Synchronized(&pThis->mLock) {
            routine = (PFUNC)pThis->mRoutine;
            threadParam = pThis->mParam;
        }

        if (routine != NULL)
            routine(threadParam);

        pThis->mThreadPool->MoveToIdleList(pThis);
        pThis->mWait = TRUE;
    }

    if (!pThis->mWait)
        pThis->mThreadPool->MoveToIdleList(pThis);

    return 0;
}

Sp<ThreadPool> ThreadPool::Create(
    IN Int32 maxThreadCount,
    IN Int32 threadCount)
{
    Sp<ThreadPool> object = new ThreadPool();
    if (!object)
        return NULL;

    if (AFAILED(object->Initialize(maxThreadCount, threadCount)))
        return NULL;

    return object;
}

ThreadPool::~ThreadPool()
{
    Sp<ThreadEx> thread;

    while (TRUE) {
        thread = NULL;

        Synchronized(&mLock) {
            if (mUsedThreadList.IsEmpty())
                break;

            thread = mUsedThreadList.Begin()->GetValue();
        }

        if (thread) {
            thread->Discard();
            thread->Join();
        }
        else
            break;
    }

    while (TRUE) {
        thread = NULL;

        Synchronized(&mLock) {
            if (mIdleThreadList.IsEmpty())
                break;

            thread = mIdleThreadList.Begin()->GetValue();
        }

        if (thread) {
            thread->Discard();
            thread->Join();
            mIdleThreadList.Detach(thread);
        }
        else
            break;
    }
}

ARESULT ThreadPool::RunThread(
    IN PVoid routine,
    IN PVoid param)
{
    Sp<ThreadEx> thread = NULL;

    Synchronized(&mLock) {
        if (mIdleThreadList.IsEmpty()) {
            if (mUsedThreadList.GetCount() >= mMaxThreadCount)
                return AE_BUSY;

            ARESULT hr = CreateIdleThread();
            if (AFAILED(hr))
                return hr;
        }

        thread = mIdleThreadList.Begin()->GetValue();
        mIdleThreadList.Detach(thread);
        mUsedThreadList.PushBack(thread);
    }

    thread->SetRoutine(routine, param);

    return thread->StartRoutine();
}

ARESULT ThreadPool::Initialize(
    IN Int32 maxThreadCount,
    IN Int32 threadCount)
{
    if (threadCount > maxThreadCount)
        return AE_INVALIDARG;

    mMaxThreadCount = maxThreadCount;
    ARESULT hr;

    for (Int32 i = 0; i < threadCount; i++) {
        hr = CreateIdleThread();
        if (AFAILED(hr))
            return hr;
    }

    return AS_OK;
}

ARESULT ThreadPool::CreateIdleThread()
{
    Sp<ThreadEx> thread = ThreadEx::Create(this);
    if (thread == NULL)
        return AE_OUTOFMEMORY;

    Synchronized(&mLock) {
        mIdleThreadList.PushBack(thread);
    }

    return AS_OK;
}

Void ThreadPool::MoveToIdleList(
    IN CONST Sp<ThreadEx> & thread)
{
    Synchronized(&mLock) {
        mUsedThreadList.Detach(thread);
        mIdleThreadList.PushBack(thread);
    }
}