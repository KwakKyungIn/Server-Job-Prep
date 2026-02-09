#include "pch.h"
#include "MetricsSystem.h"

#include "MetricsExporter.h"
#include "ProcessMetricsProvider.h"

MetricsSystem& MetricsSystem::Instance()
{
	static MetricsSystem instance;
	return instance;
}

void MetricsSystem::Initialize(const MetricsConfig& config)
{
	std::lock_guard<std::mutex> guard(_lock);
	if (_initialized)
		return;

	_initialized = true;
	_prefix = config.Prefix;
	_enabled = false;
	_hasCpuBaseline = false;
	_lastUserCpuSeconds = 0.0;
	_lastSystemCpuSeconds = 0.0;

	_processMetricsProvider = ProcessMetricsProvider::Create();

	_uptimeGauge = _registry.RegisterGauge(
		BuildMetricName("process_uptime_seconds"),
		"Process uptime in seconds.");
	_cpuSecondsCounter = _registry.RegisterCounter(
		BuildMetricName("process_cpu_seconds_total"),
		"Total user and system CPU time spent by the process.",
		{ "mode" });
	_residentMemoryGauge = _registry.RegisterGauge(
		BuildMetricName("process_resident_memory_bytes"),
		"Resident memory size in bytes.");

	const std::vector<double> metricsResponseBuckets = {
		MetricsMillisecondsToSeconds(1.0),
		MetricsMillisecondsToSeconds(5.0),
		MetricsMillisecondsToSeconds(10.0),
		MetricsMillisecondsToSeconds(50.0),
		MetricsMillisecondsToSeconds(100.0),
		MetricsMillisecondsToSeconds(500.0),
		MetricsMillisecondsToSeconds(1000.0),
	};

	_metricsResponseHistogram = _registry.RegisterHistogram(
		BuildMetricName("metrics_response_seconds"),
		"Time spent rendering /metrics response in seconds.",
		metricsResponseBuckets);

	if (config.Enabled == false)
	{
		std::cout << "[Metrics] Disabled by configuration." << std::endl;
		return;
	}

	_exporter.reset(new MetricsExporter(_registry));
	const bool started = _exporter->Start(
		config,
		[this]() { RefreshProcessMetrics(); },
		_metricsResponseHistogram);

	if (started)
	{
		_enabled = true;
	}
	else
	{
		_exporter.reset();
		std::cout << "[Metrics][WARN] Exporter startup failed. Service keeps running without metrics." << std::endl;
	}
}

void MetricsSystem::Shutdown()
{
	std::unique_ptr<MetricsExporter> exporterToStop;

	{
		std::lock_guard<std::mutex> guard(_lock);
		if (_initialized == false)
			return;

		_enabled = false;
		exporterToStop = std::move(_exporter);
	}

	if (exporterToStop)
		exporterToStop->Stop();

	std::lock_guard<std::mutex> guard(_lock);
	_processMetricsProvider.reset();
	_uptimeGauge.reset();
	_cpuSecondsCounter.reset();
	_residentMemoryGauge.reset();
	_metricsResponseHistogram.reset();
	_registry.Clear();

	_prefix.clear();
	_initialized = false;
	_hasCpuBaseline = false;
	_lastUserCpuSeconds = 0.0;
	_lastSystemCpuSeconds = 0.0;
}

bool MetricsSystem::IsExporterRunning() const
{
	std::lock_guard<std::mutex> guard(_lock);
	return _enabled;
}

MetricsRegistry& MetricsSystem::Registry()
{
	return _registry;
}

void MetricsSystem::RefreshProcessMetrics()
{
	ProcessMetricsProvider* provider = nullptr;
	std::shared_ptr<Gauge> uptimeGauge;
	std::shared_ptr<Counter> cpuCounter;
	std::shared_ptr<Gauge> residentMemoryGauge;

	{
		std::lock_guard<std::mutex> guard(_lock);
		if (_processMetricsProvider == nullptr)
			return;

		provider = _processMetricsProvider.get();
		uptimeGauge = _uptimeGauge;
		cpuCounter = _cpuSecondsCounter;
		residentMemoryGauge = _residentMemoryGauge;
	}

	if (provider == nullptr || uptimeGauge == nullptr || cpuCounter == nullptr || residentMemoryGauge == nullptr)
		return;

	ProcessMetricsSnapshot snapshot;
	if (provider->Collect(snapshot) == false)
		return;

	uptimeGauge->Set(snapshot.uptimeSeconds);
	residentMemoryGauge->Set(static_cast<double>(snapshot.residentMemoryBytes));

	double userDelta = snapshot.userCpuSeconds;
	double systemDelta = snapshot.systemCpuSeconds;

	{
		std::lock_guard<std::mutex> guard(_lock);
		if (_hasCpuBaseline)
		{
			userDelta = (snapshot.userCpuSeconds >= _lastUserCpuSeconds)
				? (snapshot.userCpuSeconds - _lastUserCpuSeconds)
				: 0.0;
			systemDelta = (snapshot.systemCpuSeconds >= _lastSystemCpuSeconds)
				? (snapshot.systemCpuSeconds - _lastSystemCpuSeconds)
				: 0.0;
		}
		else
		{
			userDelta = (snapshot.userCpuSeconds >= 0.0) ? snapshot.userCpuSeconds : 0.0;
			systemDelta = (snapshot.systemCpuSeconds >= 0.0) ? snapshot.systemCpuSeconds : 0.0;
		}

		_lastUserCpuSeconds = snapshot.userCpuSeconds;
		_lastSystemCpuSeconds = snapshot.systemCpuSeconds;
		_hasCpuBaseline = true;
	}

	if (userDelta > 0.0)
		cpuCounter->Inc(userDelta, { { "mode", "user" } });
	if (systemDelta > 0.0)
		cpuCounter->Inc(systemDelta, { { "mode", "system" } });
}

std::string MetricsSystem::BuildMetricName(const std::string& metricBaseName) const
{
	if (_prefix.empty())
		return metricBaseName;
	return _prefix + metricBaseName;
}
