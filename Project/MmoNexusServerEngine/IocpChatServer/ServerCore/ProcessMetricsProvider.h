#pragma once

#include <cstdint>
#include <memory>

struct ProcessMetricsSnapshot
{
	double uptimeSeconds = 0.0;
	double userCpuSeconds = 0.0;
	double systemCpuSeconds = 0.0;
	uint64_t residentMemoryBytes = 0;
};

class ProcessMetricsProvider
{
public:
	virtual ~ProcessMetricsProvider() = default;
	virtual bool Collect(ProcessMetricsSnapshot& outSnapshot) = 0;

	static std::unique_ptr<ProcessMetricsProvider> Create();
};
