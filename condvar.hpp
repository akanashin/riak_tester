/* Taken from http://vichargrave.com/condition-variable-class-in-c/
 *
 */
#include <pthread.h>

class mutex_t
{
    friend class condvar_t;
    pthread_mutex_t  m_mutex;

  public:
    // just initialize to defaults
    mutex_t() { pthread_mutex_init(&m_mutex, NULL); }
    virtual ~mutex_t() { pthread_mutex_destroy(&m_mutex); }

    int lock() { return  pthread_mutex_lock(&m_mutex); }
    int trylock() { return  pthread_mutex_lock(&m_mutex); }
    int unlock() { return  pthread_mutex_unlock(&m_mutex); }   
};

class condvar_t
{
    pthread_cond_t  m_cond;
    mutex_t&        m_lock;

  public:
    // just initialize to defaults
    condvar_t(mutex_t& mutex) : m_lock(mutex) { pthread_cond_init(&m_cond, NULL); }
    virtual ~condvar_t() { pthread_cond_destroy(&m_cond); }

    int wait() { return  pthread_cond_wait(&m_cond, &(m_lock.m_mutex)); }
    int signal() { return pthread_cond_signal(&m_cond); } 
    int broadcast() { return pthread_cond_broadcast(&m_cond); } 

  private:
    condvar_t();
};
