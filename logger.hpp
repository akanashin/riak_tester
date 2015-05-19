/*
 * logger.hpp
 *
 *  Created on: Sep 19, 2014
 *      Author: akanashin
 */

#ifndef LOGGER_HPP_
#define LOGGER_HPP_

#include <iostream>
#include <string>

using std::endl;

enum LogPriority {
    kLogEmerg   = 0,   // system is unusable
    kLogAlert,         // action must be taken immediately
    kLogCrit,          // critical conditions
    kLogErr,           // error conditions
    kLogWarning,       // warning conditions
    kLogNotice,        // normal, but significant, condition
    kLogInfo,          // informational message
    kLogDebug          // debug-level message
};

// some shortcuts
#define LOG   std::clog << kLogInfo
#define LOG_D std::clog << kLogDebug
#define LOG_E std::clog << kLogErr
#define LOG_W std::clog << kLogWarning

std::ostream& operator<< (std::ostream& os, const LogPriority& log_priority);

class Log : public std::basic_streambuf<char, std::char_traits<char> > {
public:
    explicit Log(std::string ident, int facility);

protected:
    int sync();
    int overflow(int c);

private:
    friend std::ostream& operator<< (std::ostream& os, const LogPriority& log_priority);
    std::string buffer_;
    int facility_;
    int priority_;
    char ident_[50];
};

void setup_logger(std::string const& prog_name, bool a_syslog);

#endif /* LOGGER_HPP_ */
