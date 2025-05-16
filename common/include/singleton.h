#pragma once

#include <concepts>
template<typename T>
class Singleton {
protected:
	// Protected constructors for derived classes
	Singleton() = default;
	virtual ~Singleton() = default;

	// Delete copy and move semantics
	Singleton(const Singleton&) = delete;
	Singleton& operator=(const Singleton&) = delete;
	Singleton(Singleton&&) = delete;
	Singleton& operator=(Singleton&&) = delete;

public:
	// Thread-safe getInstance using C++11 static local variable
	static T& getInstance() {
		static T instance;
		return instance;
	}

	static T* getInstancePtr() {
		return &getInstance();
	}
};

#define SINGLETON_INIT(ClassType) \
    friend Singleton<ClassType>; 
