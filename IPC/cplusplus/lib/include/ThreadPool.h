
#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include "AXP/cplusplus/xplatform/include/aresult.h"
#include "AXP/cplusplus/xplatform/include/list.h"
#include "AXP/cplusplus/xplatform/include/event.h"

#define DEFAULT_THREAD_MAXCOUNT (20) // 线程池默认最大线程数
#define DEFAULT_THREAD_COUNT (5) // 线程池默认线程数

namespace IPC
{
    class ThreadPool : public AXP::CObject
    {
    private:

        class ThreadEx : public AXP::CObject
        {
        public:

            STATIC AXP::Sp<ThreadEx> Create(
                IN CONST AXP::Sp<ThreadPool> & pool);

            VIRTUAL ~ThreadEx();

        public:

            AXP::Void SetRoutine(
                IN AXP::PVoid routine,
                IN AXP::PVoid param);

            AXP::ARESULT StartRoutine();

            AXP::ARESULT Discard();

            AXP::ARESULT Join();

        private:

            AXP::ARESULT Initialize(
                IN CONST AXP::Sp<ThreadPool> & pool);

            STATIC AXP::ULong STDCALL Routine(
                IN AXP::PVoid param);

        private:

            ThreadEx();

        private:

#ifdef PLATFORM_WIN32
            AXP::DWord mId;
            AXP::Handle mHandle;
#elif defined(PLATFORM_IOS) || defined(PLATFORM_LINUX)
            pthread_t mHandle;
            AXP::Boolean mThreadExists;
#endif
            AXP::Wp<ThreadPool> mThreadPool; // 所属线程池
            AXP::PVoid mRoutine; // 指向其他位置的函数
            AXP::PVoid mParam; // 函数参数
            AXP::Boolean mRunFlag; // 线程运行标志
            AXP::Boolean mWait; // 线程函数状态
            AXP::Sp<AXP::Event> mEvent; // 线程函数激活事件
        };

    public:

        STATIC AXP::Sp<ThreadPool> Create(
            IN AXP::Int32 maxThreadCount = DEFAULT_THREAD_MAXCOUNT,
            IN AXP::Int32 threadCount = DEFAULT_THREAD_COUNT);
        VIRTUAL ~ThreadPool();

    public:

        AXP::ARESULT RunThread(
            IN AXP::PVoid routine,
            IN AXP::PVoid param);

    private:

        AXP::ARESULT Initialize(
            IN AXP::Int32 maxThreadCount,
            IN AXP::Int32 threadCount);

        AXP::ARESULT CreateIdleThread();

        AXP::Void MoveToIdleList(
            IN CONST AXP::Sp<ThreadEx> & thread);

    private:

        ThreadPool()
        {
        }

    private:

        AXP::Int32 mMaxThreadCount; // 最大线程数
        AXP::List<ThreadEx> mUsedThreadList; // 运行线程的链表
        AXP::List<ThreadEx> mIdleThreadList; // 空闲线程的链表
    };
}

#endif // __THREADPOOL_H__
