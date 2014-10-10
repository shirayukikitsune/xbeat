//===-- Dispatcher.cpp - Defines a class for multithreading support ----*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===-----------------------------------------------------------------------------===//
///
/// \file
/// \brief This file defines everything related to the event dispatcher class, which
/// takes responsibility for multithreading.
///
//===-----------------------------------------------------------------------------===//

#include "Dispatcher.h"
#include <cassert>

Dispatcher::Dispatcher()
{
}

Dispatcher::~Dispatcher()
{
	shutdown();
}

void Dispatcher::initialize()
{
	assert(Run == false);

	Run = true;
	auto ThreadCount = std::thread::hardware_concurrency();
	ThreadPool.resize(ThreadCount);

	for (auto &Thread : ThreadPool) {
		Thread = std::thread(std::bind(&Dispatcher::consumeTask, this));
	}
}

void Dispatcher::shutdown()
{
	assert(Run == true);

	Run = false;

	SynchronizingCondition.notify_all();

	for (auto &Thread : ThreadPool)
		Thread.join();
}

void Dispatcher::addTask(std::function<void(void)> Task)
{
	std::lock_guard<std::mutex> Lock(Synchronizer);

	Tasks.push_back(Task);

	SynchronizingCondition.notify_one();
}

void Dispatcher::consumeTask()
{
	while (Run) {
		std::unique_lock<std::mutex> Lock(Synchronizer);
		while (Tasks.empty() && Run) SynchronizingCondition.wait(Lock);

		if (!Run) break;

		auto Task = Tasks.front();
		Tasks.pop_front();
		Lock.unlock();

		Task();
	}
}