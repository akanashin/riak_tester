#include <cassert>
#include <cstring>
#include <string>
#include <sstream>

#include "exception.hpp"
#include "cmd_executor.hpp"
#include "logger.hpp"
#include "utils.hpp"

#include <chrono>

template<typename TimeT = std::chrono::milliseconds>
struct measure
{
    template<typename F, typename ...Args>
    static typename TimeT::rep execution(F func, Args&&... args)
    {
        auto start = std::chrono::system_clock::now();
        func(std::forward<Args>(args)...);
        auto duration = std::chrono::duration_cast< TimeT> 
                            (std::chrono::system_clock::now() - start);
        return duration.count();
    }
};
////////////////////////////////////

// args:
//  IP:port
//  put/get
//  key
//  value
void print_usage()
{
    printf(R"XXX(
Usage:
    test IP:port GET KEY
    test IP:port PUT KEY VALUE
    test IP:port DEL KEY
    test IP:port TEST COUNT

IP:port - address of Riak node
GET/PUT/DEL/TEST - operation
KEY     - key for the operation
VALUE   - value for write
COUNT   - number of PUT/GET/DEL operations to be performed

Note: KEY and VALUE only used for 
)XXX");
}

enum OP {
    GET = 0,
    PUT,
    DEL,
    TEST
};

int
main(int   argc,
     char *argv[])
{
    if (argc != 4 && argc != 5)
    {
        print_usage();
        return 1;
    }
  
    int op = -1;
    if (strcasecmp(argv[2], "GET") == 0)
        op = GET;
    else
    if (strcasecmp(argv[2], "PUT") == 0)
        op = PUT;
    else
    if (strcasecmp(argv[2], "DEL") == 0)
        op = DEL;
    else
    if (strcasecmp(argv[2], "TEST") == 0)
        op = TEST;
    else
    ;

    // check and verify each address in addresses parameters
    strvector addrs = split(argv[1], ',');
    
    for (std::string addr : addrs)
    {
        if (!validate_address(addr))
        {
            print_usage();
            return 1;
        }
    }

    std::string
        key = argv[3],
        value = (argc == 5 ? argv[4] : "");
            
    setup_logger("riak_test", false);
    
    // create an executor to execute our Riak operations
    executor_t executor(addrs);
    
    try {
        switch(op) {
        case GET:
            printf("GET returned: %s\n", executor.get_key(key).c_str());
            break;
        case PUT:
            executor.put_key(key, value);
            break;
        case DEL:
            executor.del_key(key);
            break;
        case TEST:
            // testing
        {
            int count = stoi(key);
            int seed = std::rand();

            // preparing keys and values
            std::vector<std::string> keys, values;
            for(int i = 0; i < count; i++)
            {
                keys.push_back(std::string("key") + std::to_string(seed + i));
                values.push_back(std::string("value") + std::to_string(seed + i));
            }
            
            int errors = 0;
            
            printf("Performing test for %i operations\n", count);
            // create some amount of keys
            int put_time = measure<>::execution(
                [&executor, &keys, &values] () -> void
                {
                    for(int i = 0; i < keys.size(); i++)
                        executor.put_key(keys[i], values[i]);
                    executor.sync();
                } );
            
            // read some amount of keys
            int get_time = measure<>::execution(
                [&executor, &keys, &values, &errors] () -> void
                {
                    for(int i = 0; i < keys.size(); i++)
                    {
                        if (values[i] != executor.get_key(keys[i]))
                            errors++;
                    }
                    executor.sync();
                } );

            // delete created amount of keys
            int del_time = measure<>::execution(
                [&executor, &keys] () -> void
                {
                    for(int i = 0; i < keys.size(); i++)
                        executor.del_key(keys[i]);
                    executor.sync();
                } );
            
            printf("Finished in %i seconds with %i erros\n", (put_time + get_time + del_time)/1000, errors );
            printf("  PUT     : %i seconds\n", put_time/1000 );
            printf("  GET     : %i seconds\n", get_time/1000 );
            printf("  DELETE  : %i seconds\n", del_time/1000 );
        }   
            break;
        default:
            assert(!"Logical error: unknown RIAK operation");
        }

    } catch (std::exception const& ex) {
        printf("Exception: %s\n", ex.what());
        return 1;
    } catch (...) {
        printf("Unknown exception!");
        return 1;
    }

    executor.stop(false);

    return 0;
}

