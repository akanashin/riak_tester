/*
 * logger.cpp
 *
 *  Created on: Sep 19, 2014
 *      Author: akanashin
 */

#include "logger.hpp"

#include <syslog.h>
#include <cstdio>
#include <cstring>

static bool out_to_syslog = false;

static unsigned levels[] =
{
		LOG_EMERG,
	    LOG_ALERT,
	    LOG_CRIT,
	    LOG_ERR,
	    LOG_WARNING,
	    LOG_NOTICE,
	    LOG_INFO,
	    LOG_DEBUG
};

// Code by stackoverflow.com/eater
Log::Log(std::string ident, int facility) {
    facility_ = facility;
    priority_ = LOG_DEBUG;
    strncpy(ident_, ident.c_str(), sizeof(ident_));
    ident_[sizeof(ident_)-1] = '\0';

    openlog(ident_, LOG_PID, facility_);
}

int Log::sync() {
    if (buffer_.length())
    {
        if (out_to_syslog)
            syslog(priority_, buffer_.c_str());
        else
            std::cout << buffer_;

        buffer_.erase();
        priority_ = LOG_DEBUG; // default to debug for each message
    }
    return 0;
}

int Log::overflow(int c) {
    if (c != EOF) {
        buffer_ += static_cast<char>(c);
    } else {
        sync();
    }
    return c;
}

std::ostream& operator<< (std::ostream& os, const LogPriority& log_priority) {
    static_cast<Log *>(os.rdbuf())->priority_ = levels[log_priority];
    return os;
}

void setup_logger(std::string const& name, bool a_syslog)
{
    out_to_syslog = a_syslog;

	std::clog.rdbuf(new Log(name, LOG_LOCAL0));
}
