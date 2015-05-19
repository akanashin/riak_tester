#include "riak_riack.hpp"

extern "C" {
#include "riack.h"
}

#include <cstring>
#include <cassert>

#include "exception.hpp"

// wrapper for easy intialization/deletion
struct riack_string_wrap: public riack_string {
    riack_string_wrap() {
        value = 0;
    };
    riack_string_wrap(const char* str) {
        value = strdup(str);
        len = strlen(value);
    };
    riack_string_wrap(std::string const& str) {
        value = strdup(str.c_str());
        len = strlen(value);
    };

    riack_string_wrap& operator=(const riack_string_wrap& other) {
        free(value);
        
        value = strdup(other.value);
        len = strlen(value);

        return *this;
    };
    
    ~riack_string_wrap() {
        free(value);
    };
};

struct context_t {
    riack_client      *client;

    riack_string_wrap  content_type;
    riack_string_wrap  bucket;
};

riak::riak(std::string host, int portnum)
    : m_ctx( new context_t )
{
    m_ctx->bucket       = riack_string_wrap("test");
    m_ctx->content_type = riack_string_wrap("text/plain");

    riack_init();

    m_ctx->client = riack_new_client(0);
    if (!m_ctx->client
        || (riack_connect(m_ctx->client, host.c_str(), portnum, 0) != RIACK_SUCCESS))
    {
        cleanup();
        
        throw Exception("Failed to connect to RIAK server using address <" + host + ":" + std::to_string(portnum) + ">");
    }

}

riak::~riak()
{
    cleanup();
}

void riak::cleanup()
{
    riack_free(m_ctx->client);
	riack_cleanup();

    delete m_ctx;
}

bool riak::reconnect()
{
    return (riack_reconnect(m_ctx->client) == RIACK_SUCCESS);
}

int riak::put_key(std::string const& key, std::string const& value)
{
    if (key.empty() || value.empty())
        assert(!"put_key received empty pointer(s)");

    /*
    riack_string_wrap key_(key);
    riack_string_wrap value_(value);
    
    return riack_put_simple(ctx.client, ctx.bucket.value, key_.value, (uint8_t*)value_.value, value_.len, ctx.content_type.value);
    */
    riack_object object   {0};
    riack_content content {0};

    // Note: these const casts are Ok here - this data is not changing inside riack_put
    object.bucket    = m_ctx->bucket;
    object.key.value = const_cast<char*>(key.c_str());
    object.key.len   = key.size();
    object.content   = &content;
    
    content.content_type = m_ctx->content_type;
    content.data     = (uint8_t*)(value.c_str());
    content.data_len = value.size();

    return riack_put(m_ctx->client, &object, 0, 0);
}

int riak::get_key(std::string const& key, std::string *value)
{
    if (key.empty() || !value)
        assert(!"get_key received empty pointer(s)");

	riack_get_object *obj = 0;
	riack_string_wrap key_(key);

    int result = riack_get(m_ctx->client, &m_ctx->bucket, &key_, 0, &obj);
    if (result == RIACK_SUCCESS)
    {
        // todo: conflict resolution?
        if (obj->object.content_count == 1
            && obj->object.content[0].data_len > 0)
        {
            *value = std::string((const char*)obj->object.content[0].data, obj->object.content[0].data_len);
        }
    }

    riack_free_get_object_p(m_ctx->client, &obj);

	return result;
}

int riak::del_key(std::string const& key)
{
    if (key.empty())
        assert(!"del_key received empty pointer(s)");
    
	riack_string_wrap key_(key);

	return riack_delete(m_ctx->client, &m_ctx->bucket, &key_, 0);
}

bool riak::is_error_code(int code)
{
    return code == (RIACK_ERROR_COMMUNICATION) || (code == RIACK_FAILED_PB_UNPACK);
}

riak_iface_ptr create_riak_instance(std::string host, int portnum)
{
    return riak_iface_ptr( new riak(host, portnum) );
}
