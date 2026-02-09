#pragma once

#include <memory>
#include <mutex>
#include <string>

#include "CoreGlobal.h"
#include "Metrics.h"

class MetricsExporter;
class ProcessMetricsProvider;

class MetricsSystem
{
public:
	static MetricsSystem& Instance();

	void Initialize(const MetricsConfig& config);
	void Shutdown();

	bool IsExporterRunning() const;
	MetricsRegistry& Registry();
	std::shared_ptr<Counter> RegisterCounter(
		const std::string& metricBaseName,
		const std::string& help,
		const std::vector<std::string>& labelNames = {});
	std::shared_ptr<Gauge> RegisterGauge(
		const std::string& metricBaseName,
		const std::string& help,
		const std::vector<std::string>& labelNames = {});
	std::shared_ptr<Histogram> RegisterHistogram(
		const std::string& metricBaseName,
		const std::string& help,
		const std::vector<double>& buckets,
		const std::vector<std::string>& labelNames = {});

	void RefreshProcessMetrics();

private:
	MetricsSystem() = default;
	~MetricsSystem() = default;

	std::string BuildMetricName(const std::string& metricBaseName) const;

private:
	mutable std::mutex _lock;
	bool _initialized = false;
	bool _enabled = false;
	std::string _prefix;

	MetricsRegistry _registry;
	std::unique_ptr<ProcessMetricsProvider> _processMetricsProvider;
	std::unique_ptr<MetricsExporter> _exporter;

	std::shared_ptr<Gauge> _uptimeGauge;
	std::shared_ptr<Counter> _cpuSecondsCounter;
	std::shared_ptr<Gauge> _residentMemoryGauge;
	std::shared_ptr<Histogram> _metricsResponseHistogram;

	bool _hasCpuBaseline = false;
	double _lastUserCpuSeconds = 0.0;
	double _lastSystemCpuSeconds = 0.0;
};
