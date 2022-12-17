#include <cstdio>
#include <string>
#include <stdexcept>
#include <chrono>

#include "logger.h"

using Clock = std::chrono::steady_clock;

Logger::Logger(FILE *_file, log_level _min_level) {
	//Check if fileptr is valid
	if (!_file) {
		throw std::runtime_error("Invalid file pointer provided");
	}

	//Initialize members
	file = _file;
	min_level = _min_level;
	start_time = Clock::now();
}

void Logger::log(log_level level, std::string message) {
	std::string level_string;
	Clock::time_point now;
	std::chrono::milliseconds duration;

	//Check if we can exit early
	if (level < min_level) {
		return;
	}


	//Determine time of log
	now = Clock::now();
	duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);

	//Determine warning level
	switch (level) {
		case debug:
			level_string = "DEBUG";
			break;
		case info:
			level_string = "INFO";
			break;
		case warning:
			level_string = "WARNING";
			break;
		case error:
			level_string = "ERROR";
			break;
		case critical:
			level_string = "CRITICAL";
			break;
		default:
			throw std::runtime_error("Unknown log level: " + level);
			break;
	}

	//Print log
	fprintf(file, "%ld:%s:%s\n", duration.count(), level_string.c_str(), message.c_str());
	fflush(file);
}

