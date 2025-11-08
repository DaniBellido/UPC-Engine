#pragma once

#include <string>
#include <vector>

enum LogType 
{
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR
};

struct LogEntry 
{
	LogType type;
	std::string message;
};

class Logger
{
private:
	static std::vector<LogEntry> messages;

	static std::string getTime();

public:
	
	static void Log(const std::string& message);
	static void Err(const std::string& message);
	static void Warn(const std::string& message);
	static void Clear();
	static const std::vector<LogEntry>& GetMessages() { return messages; }
};

