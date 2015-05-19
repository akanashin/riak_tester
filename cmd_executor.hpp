#include "queue.hpp"

#include <vector>

typedef std::vector<std::string> strvector;

struct command_t;

class executor_t {
public: 
    executor_t(strvector const& addrlist);
    ~executor_t();

    bool put_key(std::string const& key, std::string const& value);
    std::string get_key(std::string const& key);
    bool del_key(std::string const& key);

    // pauses client until executor finished its queue
    void sync();
    
    // true - stop right now (ignore commands in queue)
    // false - stop when done (queue is empty)
    void stop(bool stop_now);

    // true is executor is stoped
    bool is_stoped() const;
    
private:
    struct impl_t;
    impl_t *m_impl;
};
