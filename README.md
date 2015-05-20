# riak_tester
Extensible C++ wrapper and performance tester for various Riak client libraries

Can performs single PUT/GET/DELETE operation and performance test of number of PUT/GET/DELETE operations.
Can use group of RIAK cluster entries.

Currently uses riack_master C library and underhood protocol library. Can be used with any other Riak C/C++ client library (need to implement interface riak_iface).

Files:
- test.cpp
   main entry point. Reads command line arguments and performs test operations.
- cmd_processor {hpp,cpp}
   Riak command processor for PUT/GET/DELETE commands. Implements asynchronous execution of commands and Riak connection pooling.
- queue, logger, utils, exception, condvar
   Various helpers
- riak_iface.hpp
   Common interface Riak client library must implement to be used with this complex
- riak_riack {hpp,cpp}
   Adapter for riack_master Riak C library
- Makefile
   makefile for make


   
