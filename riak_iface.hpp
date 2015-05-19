#ifndef RIAK_IFACE_HPP
#define RIAK_IFACE_HPP

#include <string>
#include <memory>

class riak_iface {
public:
    virtual ~riak_iface() {};
    virtual bool reconnect() = 0;
    
    virtual int put_key(std::string const& key, std::string const& value) = 0;
    virtual int get_key(std::string const& key, std::string *value) = 0;
    virtual int del_key(std::string const& key) = 0;

    virtual bool is_error_code(int code) = 0;
};
typedef std::shared_ptr<riak_iface> riak_iface_ptr;

riak_iface_ptr create_riak_instance(std::string host, int portnum);

#endif //RIAK_IFACE_HPP
