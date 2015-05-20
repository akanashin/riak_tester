#include "cmd_executor.hpp"

#include <pthread.h>
#include <cassert>
#include <atomic>
#include <unistd.h>

#include "exception.hpp"
#include "logger.hpp"
#include "condvar.hpp"
#include "utils.hpp"

#include "riak_iface.hpp"


#define CHECK(CMD) do{ \
    int s = CMD;       \
    if (s != 0)        \
        throw Exception( #CMD " failed with code: " + std::to_string(s)); \
    } while(0)

struct command_t {
    enum class op_e {
        PUT = 0,
        GET,
        DELETE,
    }           type;
    
    std::string key;
    
    // ignored for GET or DEL operations
    std::string value;

    // callback is used for GET operations
    std::function<void(std::string)> cb;
};

struct executor_t::impl_t {
    queue<command_t, strategy_drop_first<command_t> >     m_queue;
    strvector            m_addrs;

    size_t               m_cmd_executor_thr_id;
    size_t               m_reconnector_thr_id;


    // general flag which indicates that everything goes down
    std::atomic_bool     m_stoping;
    
    std::vector<riak_iface_ptr> m_riaks;

    // group of queues used for asynchronous reconnect of Riak clienhts
    //  these queues used between Command processor and Reconnector
    queue<riak_iface_ptr>  m_to_reconnect;
    queue<riak_iface_ptr>  m_from_reconnect;

    //
    void start_thread();
    void stop_thread(bool stop_now);
    bool is_thread_active() const;

    void exec(command_t const& cmd);

    // Main work cycle - processing of commands
    enum class mode_e {
        RUN = 0,
        STOP_WHEN_DONE,
        STOP_NOW
    };
    std::atomic<mode_e>  m_cur_mode;

    // main threads
    void cmd_processor();
    void reconnector();

    // Reconnector thread
    void start_reconnector_thread();

    // thread helpers
    static void* thr_cmd_processor(void *arg);
    static void* thr_reconnector(void *arg);
};
//


executor_t::executor_t(strvector const& addrlist)
    : m_impl(new impl_t)
{
    // create Riak instances
    if (addrlist.empty())
        throw Exception("executor_t got empty list of addresses");

    m_impl->m_cmd_executor_thr_id = 0;
    m_impl->m_reconnector_thr_id = 0;
    m_impl->m_cur_mode.store(impl_t::mode_e::RUN);
    m_impl->m_stoping.store(false);

    for(std::string const& addr : addrlist)
    {
        std::string host;
        int port;
        if (!validate_address(addr, &host, &port))
        {
            LOG_W << "Ignored incorrect Riak address <" + addr + ">";
            continue;
        }

        // create instance
        LOG << "Creating RIAK client for " << addr << endl;
        m_impl->m_riaks.push_back( create_riak_instance(host, port) );
    }

    // check if there is no Riak clients was created
    if (m_impl->m_riaks.empty())
        throw Exception("No Riak clients can be created");
    
    // start threads
    m_impl->start_thread();
    m_impl->start_reconnector_thread();
}

executor_t::~executor_t()
{
    stop(true);
}

void executor_t::stop(bool stop_now)
{
    LOG_D << "Command to stop " << (stop_now ? "RIGHT NOW" : "WHEN DONE") << endl;

    m_impl->m_stoping.store(true);
    m_impl->stop_thread(stop_now);
}

void executor_t::sync()
{
    // restart internal thread
    m_impl->stop_thread(false);
    m_impl->start_thread();
}

bool executor_t::put_key(std::string const& key, std::string const& value)
{
    if (!m_impl->is_thread_active())
        return false;
    
    command_t cmd;
    cmd.type = command_t::op_e::PUT;
    cmd.key = key;
    cmd.value = value;

    m_impl->exec(cmd);
}

std::string executor_t::get_key(std::string const& key)
{
    if (!m_impl->is_thread_active())
        return "";

    std::string result;
    mutex_t m;
    condvar_t cv(m);
    
    command_t cmd;
    cmd.type = command_t::op_e::GET;
    cmd.key = key;
    cmd.cb = [&cv, &result] (std::string const& value)
        {
            result = value;
            cv.signal();
        };

    m_impl->exec(cmd);

    cv.wait();

    return result;
}

bool executor_t::del_key(std::string const& key)
{
    if (!m_impl->is_thread_active())
        return false;

    command_t cmd;
    cmd.type = command_t::op_e::DELETE;
    cmd.key = key;

    m_impl->exec(cmd);
}
//


////////////////////////////////////////////////////////////////////////////////
// Implementation goes here
void executor_t::impl_t::start_thread()
{
    LOG_D << "Starting internal thread" << endl;
    pthread_attr_t attr;
    CHECK(pthread_attr_init(&attr));
    CHECK(pthread_create(&m_cmd_executor_thr_id, &attr,
                         [] (void *arg) -> void* { static_cast<impl_t*>(arg)->cmd_processor(); },
                         this));
    
    CHECK(pthread_attr_destroy(&attr));

    m_cur_mode.store(mode_e::RUN);
}

void executor_t::impl_t::start_reconnector_thread()
{
    LOG_D << "Starting reconnector thread" << endl;
    pthread_attr_t attr;
    CHECK(pthread_attr_init(&attr));
    CHECK(pthread_create(&m_reconnector_thr_id, &attr,
                         [] (void *arg) -> void* { static_cast<impl_t*>(arg)->reconnector(); },
                         this));
    CHECK(pthread_attr_destroy(&attr));
}

void executor_t::impl_t::stop_thread(bool stop_now)
{
    LOG_D << "New mode: " << (stop_now ? "STOP_NOW" : "STOP_WHEN_DONE") << endl;
    m_cur_mode.store(stop_now ? mode_e::STOP_NOW : mode_e::STOP_WHEN_DONE);
    pthread_join(m_cmd_executor_thr_id, 0);
    m_cmd_executor_thr_id = 0;
}

bool executor_t::impl_t::is_thread_active() const
{
    return m_cmd_executor_thr_id != 0;
}

void executor_t::impl_t::exec(command_t const& cmd)
{
    m_queue.enqueue(cmd);
}

// threads
void executor_t::impl_t::cmd_processor()
{
    LOG_D << "Main thread started" << endl;

    // this try/catch is only needed for logging
    //  (i want to see if thread if finished in any case)
    try
    {       
        while ( m_cur_mode.load() != mode_e::STOP_NOW )
        {
            // waiting for alive Riak clients
            if (m_riaks.empty()) {
                LOG_D << "no Riak clients!" << endl;

                // trying to get alive Riak client from Reconnector thread
                riak_iface_ptr p;
                if ( m_from_reconnect.dequeue(p, 1000) )
                    m_riaks.push_back(p);
                    
                continue;
            }
            
            
            // processign next command
            command_t cmd;

            // check if we need to stop right now/if there is no data
            if (!m_queue.dequeue(cmd, 1000))
            {
                // queue is empty. Do we need to stop?
                if (m_cur_mode.load() == mode_e::STOP_WHEN_DONE)
                    break;

                LOG_D << ".. timeout" << endl;
                continue;
            }

            // check if we need to stop in any case
            if (m_cur_mode.load() == mode_e::STOP_NOW)
                break;

            // always use the first Riak client
            riak_iface_ptr p = m_riaks.front();

            int result = 0;
            std::string str_result;
            switch(cmd.type)
            {
            case command_t::op_e::PUT:
                result = p->put_key(cmd.key, cmd.value);
                break;
            case command_t::op_e::GET:
                result = p->get_key(cmd.key, &str_result);
                break;
            case command_t::op_e::DELETE:
                result = p->del_key(cmd.key);
                break;
            default:
                assert(!"Unknown type of operation");
            }

            // verify result
            if (p->is_error_code(result))
            {
                LOG_D << "Send client to reconnect (result code=" << result << ")" << endl;

                // reconnect current client
                //  (send it to reconnector thread)
                m_to_reconnect.enqueue(p);
                m_riaks.erase(m_riaks.begin());

                // put command back to queue to repeat executing later
                m_queue.enqueue(cmd);
                
                continue;
            }

            // command executed successfully
            // return result of --successfull-- GET operation
            if (cmd.type == command_t::op_e::GET)
                try {
                    cmd.cb(str_result);
                } catch (...) {}
        }

    } catch (std::exception const& ex) {
        LOG_E << "Error: " << ex.what() << endl;
    } catch (...) {
        LOG_E << "Unknown exception!" << endl;
    }
    
    LOG_D << "Main thread stoped" << endl;
}

void executor_t::impl_t::reconnector()
{
    LOG_D << "Reconnector thread started" << endl;
    
    while ( !m_stoping.load() )
    {
        riak_iface_ptr p;

        if (!m_to_reconnect.dequeue(p, 1000))
            continue;

        // there is client to reconnect
        LOG_D << "Reconnecting client... ";

        if (p->reconnect())
        {
            LOG_D << "Done" << endl;
            m_from_reconnect.enqueue(p);
        } else
        {
            LOG_D << "Failure" << endl;

            // return client back to queue
            m_to_reconnect.enqueue(p);

            // delay to prevent 100% of CPU load
            sleep(5);
        }
    }

    LOG_D << "Reconnector thread stoped" << endl;
}
