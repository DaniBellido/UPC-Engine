#pragma once

#include <string>
#include <vector>

//-----------------------------------------------------------------------------
// A centralized static logging system for the engine or application.
//
// Features:
// -Supports three log levels : LOG_INFO, LOG_WARNING, LOG_ERROR.
// -Records timestamped messages in an internal vector(LogEntry).
// -Provides static methods for logging messages anywhere without creating an instance :
// -Log() : Records informational messages.
// -Warn() : Records warnings.
// -Err() : Records errors.
// -Clear() : Clears all stored messages.
// -GetMessages() allows access to all stored log entries, useful for consoles, debug windows, or runtime inspection.
//
// Usage Example :
// Logger::Log("Engine initialized.");
// Logger::Warn("Low memory detected.");
// Logger::Err("Failed to load texture.");
//-----------------------------------------------------------------------------

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

