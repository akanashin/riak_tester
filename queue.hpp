#ifndef __QUEUE_HPP__
#define __QUEUE_HPP__

#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <chrono>


template <class T>
class queue
{
public:
    void enqueue(const T & cdata);
    void dequeue(T & cdata);
    bool dequeue(T & cdata, const int timeout_ms);
    void clear();
private:
    std::deque<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cond_var;
};


template <class T>
void queue<T>::enqueue(const T & cdata)
{
    bool was_empty;
    
    {
        // The m_mutex must be unlocked when we call notify_one().
        std::lock_guard<std::mutex> lock(m_mutex);
        was_empty = m_queue.empty();
        m_queue.push_back(cdata);
    }
    
    if (was_empty)
        m_cond_var.notify_one();
}

template <class T>
void queue<T>::dequeue(T & cdata)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cond_var.wait(lock, [this] { return !m_queue.empty(); });
    cdata = m_queue.front();
    m_queue.pop_front();
}

template <class T>
bool queue<T>::dequeue(T & cdata, const int timeout_ms)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    
    // According to the standard conditional_variables are allowed to wakeup spuriously, even
    // if the event hasn't occured. In case of a spurious wakeup it will return cv_status::no_timeout,
    // even though it hasn't been notified.
    // See 30.5.1
    // We will continue waiting if m_queue is empty. If m_queue is not empty we won't call any "wait" functions.
    if (!m_cond_var.wait_until(lock,
            std::chrono::system_clock::now() + std::chrono::milliseconds(timeout_ms),
            [this] { return !m_queue.empty(); }))
    {
        // The timeout "timeout_ms" has expired. m_queue still empty.
        return false;
    }
    
    cdata = m_queue.front();
    m_queue.pop_front();
    return true;
}

template <class T>
void queue<T>::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.clear();
}

#endif // __QUEUE_H__

