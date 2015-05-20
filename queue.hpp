#ifndef __QUEUE_HPP__
#define __QUEUE_HPP__

#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <chrono>


template <typename T>
struct strategy_drop_last;
template <typename T>
struct strategy_drop_first;

template <class T, class S = strategy_drop_last<T> >
class queue
{
public:
    queue(int max_length = -1): m_max_length(max_length) {}

    // returns true is success, false if element was not enqueued
    bool enqueue(const T & cdata);

    void dequeue(T & cdata);
    bool dequeue(T & cdata, const int timeout_ms);
    void clear();
    
private:
    std::deque<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cond_var;
    int m_max_length;
};

template <typename T>
struct strategy_drop_last {
    static bool fix(queue<T, strategy_drop_last<T> >*, T const&) { return false; };
};
template <typename T>
struct strategy_drop_first {
    static bool fix(queue<T, strategy_drop_first<T> >* q, T const& t) {
        T tmp;
        q->dequeue(tmp);
        q->enqueue(t);

        return true;
    };
};

// returns false is queue is already full
//         true is cdata was put into queue
template <typename T, typename S>
bool queue<T, S>::enqueue(const T & cdata)
{
    bool was_empty;
    bool result = true;
    
    {
        // The m_mutex must be unlocked when we call notify_one().
        std::lock_guard<std::mutex> lock(m_mutex);
        was_empty = m_queue.empty();

        // check if queue is full
        if (m_max_length != -1 && m_queue.size() >= m_max_length)
        {
            result = S::fix(this, cdata);
        } else
            m_queue.push_back(cdata);
    }
    
    if (was_empty)
        m_cond_var.notify_one();

    return result;
}

template <typename T, typename S>
void queue<T, S>::dequeue(T & cdata)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cond_var.wait(lock, [this] { return !m_queue.empty(); });
    cdata = m_queue.front();
    m_queue.pop_front();
}

template <typename T, typename S>
bool queue<T, S>::dequeue(T & cdata, const int timeout_ms)
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

template <typename T, typename S>
void queue<T, S>::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.clear();
}

#endif // __QUEUE_H__

