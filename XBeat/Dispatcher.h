//===-- Dispatcher.h - Declares a class for multithreading support ----*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------------===//
///
/// \file
/// \brief This file declares everything related to the event dispatcher class, which
/// takes responsibility for multithreading.
///
//===----------------------------------------------------------------------------===//

#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

/// \brief Class to facilitate access for multithreaded tasks
class Dispatcher
{
public:
	Dispatcher();
	~Dispatcher();

	/// \brief Initializes the task dispatcher
	void initialize();
	/// \brief Stops the task dispatcher
	void shutdown();

	/// \brief Adds a task to the queue
	void addTask(std::function<void(void)> Task);

private:
	/// \brief The pool of running threads
	std::vector<std::thread> ThreadPool;
	/// \brief Defines whether the dispatcher is running or not
	bool Run;

	/// \brief Queue of tasks to be run
	std::deque<std::function<void(void)>> Tasks;
	/// \brief Mutex to synchronize the access to the queue of tasks
	std::mutex Synchronizer;
	/// \brief Condition variable to notify a running thread that a task entered the queue
	std::condition_variable SynchronizingCondition;
	
	/// \brief Entry point of the threads
	void consumeTask();
};

