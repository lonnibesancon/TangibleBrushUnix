#ifndef WORKER_THREAD_H
#define WORKER_THREAD_H

#include "global.h"

#include <functional>

#include "thirdparty/tinythread.h"

template <typename T>
class WorkerThread : public tthread::thread
{
public:
	// WorkerThread()
	//  : mStopped(true), mNewData(false)
	// {}

	WorkerThread(typename std::function<void (T)> func)
	 : tthread::thread(&run_, static_cast<void*>(this)),
	   mFunc(func)
	{}

	void process(const T& data)
	{
		android_assert(!mStopped);
		android_assert(joinable());
		tthread::lock_guard<tthread::mutex> g(mDataLock);
		mData = data;
		mNewData = true;
		mCond.notify_one();
	}

	void stop()
	{
		mStopped = true;
	}

	bool isWaiting()
	{
		return !mNewData;
	}

private:
	static void run_(void* this_)
	{ static_cast<WorkerThread*>(this_)->run(); }

	void run()
	{
		try {
			mStopped = false;
			mNewData = false;
			mData = T();

			while (!mStopped) {
				while (!mNewData) {
					mCond.wait(mLock);
				}

				tthread::lock_guard<tthread::mutex> g(mDataLock);
				mFunc(mData);
				mNewData = false;
			}

		} catch (const std::exception& e) {
			LOGE("Exception in worker thread: %s", e.what());

		} catch (...) {
			LOGE("Unknown exception in worker thread");
		}

		// Unblock the parent thread in case it is still waiting for
		// isWaiting() == true
		mNewData = false;
	}

	typename std::function<void (T)> mFunc;
	bool mStopped, mNewData;
	tthread::mutex mLock;
	tthread::condition_variable mCond;
	T mData;
	tthread::mutex mDataLock;
};

#endif /* WORKER_THREAD_H */
