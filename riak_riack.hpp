#include "riak_iface.hpp"

struct context_t;

class riak: public riak_iface {
public:
    riak(std::string addr, int portnum);
    ~riak();
    bool reconnect();
    
    int put_key(std::string const& key, std::string const& value);
    int get_key(std::string const& key, std::string *value);
    int del_key(std::string const& key);

    bool is_error_code(int code);

private:
    void cleanup();
    
    context_t *m_ctx;
};
