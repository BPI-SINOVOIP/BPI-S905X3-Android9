/****************************************************************************
*
*    Copyright (c) 2020 Vivante Corporation
*
*    Permission is hereby granted, free of charge, to any person obtaining a
*    copy of this software and associated documentation files (the "Software"),
*    to deal in the Software without restriction, including without limitation
*    the rights to use, copy, modify, merge, publish, distribute, sublicense,
*    and/or sell copies of the Software, and to permit persons to whom the
*    Software is furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*    DEALINGS IN THE SOFTWARE.
*
*****************************************************************************/
#ifndef _NNRT_EXECUTION_TASK_H
#define _NNRT_EXECUTION_TASK_H

#include <queue>
#include <mutex>
#include "prepared_model.hpp"
#include "execution.hpp"
#include "event.hpp"

namespace nnrt
{

enum TaskType {
    GRAPH_TASK,
};

struct Task
{
    explicit Task(/*TaskType type*/)/*: task_type_(type)*/ {}
    virtual ~Task()=default;
    virtual int execute() = 0;

private:
    // TaskType task_type_;
};

struct GraphTask: public Task {
    GraphTask(PreparedModelPtr prepared_model,
            EventPtr event,
            std::vector<ExecutionIOPtr>& inputs,
            std::vector<ExecutionIOPtr>& outputs)
        : Task(/*TaskType::GRAPH_TASK*/)
        , prepared_model_(prepared_model)
        , event_(event)
        , inputs_(inputs)
        , outputs_(outputs)
        {}

    int execute() override {
        if (event_) {
            return executeWithNotify();
        } else {
            return executeNormal();
        }
    }

private:
    /**
     * Run graph and notify event
     * If task event is not NULL, this function will be called.
     */
    int executeWithNotify();
    /**
     * Run graph
     * If task event is NULL, this function will be called.
     */
    int executeNormal();
    /**
     * Fill input data.
     * Thread safe API.
     */
    int fillInput();
    /**
     * Fill output data.
     * Thread safe API.
     */
    int fillOutput();

    PreparedModelPtr            prepared_model_;
    EventPtr                    event_;
    std::vector<ExecutionIOPtr> inputs_;
    std::vector<ExecutionIOPtr> outputs_;
};

using TaskPtr = std::shared_ptr<Task>;

class TaskQueue {
public:
    TaskQueue() {};
    /**
     * Enqueue task and try to start a work thread to run the task.
     * There is a work thread is running, the new task will be added to its queue.
     * @note Thread safe API.
     */
    int enqueue(TaskPtr task);
    /**
     * Dequeue task.
     * This function should only be called by work thread.
     * It will dequeue a task and run it.
     * @note Thread safe API.
     */
    TaskPtr dequeue();

private:
    /**
     * Put work thread state to sleep.
     * Work thread will call this when it is going to free the thread resource.
     * @note This API is protected by queue_mutex_
     */
    void exitWorkThread() {
        work_thread_awaked_ = false;
    }

    /**
     * Wake work thread
     * If work thread is finished, try to create a new work thread.
     * If work thread is running, do nothing.
     * @note This API is protected by queue_mutex_
     */
    void wakeWorkThread();

    /**
     * Flag to indicate if the work thread is running.
     */
    bool work_thread_awaked_{false};
    std::queue<TaskPtr> queue_;
    std::mutex queue_mutex_;
};

TaskQueue& get_global_task_queue();

}

#endif
