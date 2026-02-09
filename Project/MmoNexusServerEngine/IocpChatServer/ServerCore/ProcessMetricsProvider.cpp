#include "pch.h"
#include "ProcessMetricsProvider.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <psapi.h>
#pragma comment(lib, "Psapi.lib")
#else
#include <sys/types.h>
#include <unistd.h>
#endif

namespace
{
#if defined(_WIN32)
	uint64_t FileTimeToUInt64(const FILETIME& fileTime)
	{
		ULARGE_INTEGER value;
		value.LowPart = fileTime.dwLowDateTime;
		value.HighPart = fileTime.dwHighDateTime;
		return value.QuadPart;
	}

	class WindowsProcessMetricsProvider final : public ProcessMetricsProvider
	{
	public:
		bool Collect(ProcessMetricsSnapshot& outSnapshot) override
		{
			FILETIME creationTime;
			FILETIME exitTime;
			FILETIME kernelTime;
			FILETIME userTime;

			if (::GetProcessTimes(
				::GetCurrentProcess(),
				&creationTime,
				&exitTime,
				&kernelTime,
				&userTime) == FALSE)
			{
				return false;
			}

			FILETIME now;
			::GetSystemTimeAsFileTime(&now);

			const uint64_t nowTicks = FileTimeToUInt64(now);
			const uint64_t creationTicks = FileTimeToUInt64(creationTime);
			const uint64_t userTicks = FileTimeToUInt64(userTime);
			const uint64_t kernelTicks = FileTimeToUInt64(kernelTime);

			if (nowTicks > creationTicks)
			{
				outSnapshot.uptimeSeconds = static_cast<double>(nowTicks - creationTicks) / 10000000.0;
			}
			else
			{
				outSnapshot.uptimeSeconds = 0.0;
			}

			outSnapshot.userCpuSeconds = static_cast<double>(userTicks) / 10000000.0;
			outSnapshot.systemCpuSeconds = static_cast<double>(kernelTicks) / 10000000.0;

			PROCESS_MEMORY_COUNTERS_EX memoryCounters;
			::ZeroMemory(&memoryCounters, sizeof(memoryCounters));
			memoryCounters.cb = sizeof(memoryCounters);

			if (::GetProcessMemoryInfo(
				::GetCurrentProcess(),
				reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memoryCounters),
				sizeof(memoryCounters)))
			{
				outSnapshot.residentMemoryBytes = static_cast<uint64_t>(memoryCounters.WorkingSetSize);
			}
			else
			{
				outSnapshot.residentMemoryBytes = 0;
			}

			return true;
		}
	};
#else
	class LinuxProcessMetricsProvider final : public ProcessMetricsProvider
	{
	public:
		bool Collect(ProcessMetricsSnapshot& outSnapshot) override
		{
			std::ifstream statFile("/proc/self/stat");
			if (statFile.is_open() == false)
				return false;

			std::string statLine;
			std::getline(statFile, statLine);
			statFile.close();

			const size_t rightParen = statLine.rfind(')');
			if (rightParen == std::string::npos || rightParen + 2 >= statLine.size())
				return false;

			std::string trailing = statLine.substr(rightParen + 2);
			std::istringstream statStream(trailing);
			std::vector<std::string> fields;
			std::string token;
			while (statStream >> token)
				fields.push_back(token);

			if (fields.size() <= 21)
				return false;

			const long clockTicksPerSecond = ::sysconf(_SC_CLK_TCK);
			const long pageSize = ::sysconf(_SC_PAGESIZE);
			if (clockTicksPerSecond <= 0 || pageSize <= 0)
				return false;

			const double userTicks = std::stod(fields[11]);
			const double systemTicks = std::stod(fields[12]);
			const double startTicks = std::stod(fields[19]);
			const long long rssPages = std::stoll(fields[21]);

			std::ifstream uptimeFile("/proc/uptime");
			if (uptimeFile.is_open() == false)
				return false;

			double systemUptime = 0.0;
			uptimeFile >> systemUptime;
			uptimeFile.close();

			outSnapshot.userCpuSeconds = userTicks / static_cast<double>(clockTicksPerSecond);
			outSnapshot.systemCpuSeconds = systemTicks / static_cast<double>(clockTicksPerSecond);

			const double processStartSeconds = startTicks / static_cast<double>(clockTicksPerSecond);
			if (systemUptime > processStartSeconds)
				outSnapshot.uptimeSeconds = systemUptime - processStartSeconds;
			else
				outSnapshot.uptimeSeconds = 0.0;

			if (rssPages > 0)
				outSnapshot.residentMemoryBytes = static_cast<uint64_t>(rssPages) * static_cast<uint64_t>(pageSize);
			else
				outSnapshot.residentMemoryBytes = 0;

			return true;
		}
	};
#endif
}

std::unique_ptr<ProcessMetricsProvider> ProcessMetricsProvider::Create()
{
#if defined(_WIN32)
	return std::unique_ptr<ProcessMetricsProvider>(new WindowsProcessMetricsProvider());
#else
	return std::unique_ptr<ProcessMetricsProvider>(new LinuxProcessMetricsProvider());
#endif
}
