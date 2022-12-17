#ifndef LOGGER_H
#define LOGGER_H

#include <cstdio>
#include <chrono>
#include <string>

enum log_level {debug, info, warning, error, critical};

class Logger {
	public:
		Logger(FILE *fileptr, log_level min_level);
		void log(log_level level, std::string message);
	private:
		FILE *file;
		log_level min_level;
		std::chrono::steady_clock::time_point start_time;
};

#endif