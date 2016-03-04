#ifndef SYNCHRONIZED_H
#define SYNCHRONIZED_H

#include "global.h"

#include "thirdparty/tinythread.h"

template <class T>
struct Synchronized : public T
{
	Synchronized() : T() {}
	Synchronized(const T& t) : T(t) {}

	Synchronized& operator=(const T& other)
	{
		T& base = *this;
		base = other;
		return *this;
	}

	Synchronized& operator=(T&& other)
	{
		T& base = *this;
		base = std::move(other);
		return *this;
	}

	tthread::mutex mutex;
};

struct SynchronizationLock
{
	template <typename T>
	SynchronizationLock(const Synchronized<T>& t)
	 : mutex(const_cast<tthread::mutex&>(t.mutex))
	{ mutex.lock(); }

	~SynchronizationLock() { mutex.unlock(); }

	operator bool() const { return true; }
	tthread::mutex& mutex;
};

#define synchronized(obj) if (SynchronizationLock lock__ = obj)
#define synchronized_if(obj) if (obj) synchronized(obj)

#endif /* SYNCHRONIZED_H */
