#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

using MetricLabels = std::vector<std::pair<std::string, std::string>>;

enum class MetricType
{
	Counter,
	Gauge,
	Histogram,
};

double MetricsMillisecondsToSeconds(double milliseconds);

namespace MetricsHistogramBuckets
{
	const std::vector<double>& PacketHandleSeconds();
	const std::vector<double>& DbQuerySeconds();
	const std::vector<double>& JobQueueWaitSeconds();
}

class MetricBase
{
public:
	MetricBase(
		const std::string& name,
		const std::string& help,
		MetricType type,
		const std::vector<std::string>& labelNames);
	virtual ~MetricBase() = default;

	const std::string& GetName() const;
	const std::string& GetHelp() const;
	MetricType GetType() const;
	const std::vector<std::string>& GetLabelNames() const;

	virtual void AppendPrometheus(std::string& out) const = 0;

protected:
	bool BuildOrderedLabelValues(const MetricLabels& labels, std::vector<std::string>& outValues) const;
	std::string BuildSeriesKey(const std::vector<std::string>& labelValues) const;

	void AppendLabelSet(
		std::string& out,
		const std::vector<std::string>& labelValues,
		const std::pair<std::string, std::string>* extraLabel = nullptr) const;

	static std::string EscapeHelp(const std::string& value);
	static std::string EscapeLabel(const std::string& value);
	static std::string FormatDouble(double value);

private:
	std::string _name;
	std::string _help;
	MetricType _type;
	std::vector<std::string> _labelNames;
};

class Counter : public MetricBase
{
public:
	Counter(const std::string& name, const std::string& help, const std::vector<std::string>& labelNames = {});

	void Inc(double value = 1.0, const MetricLabels& labels = {});
	void AppendPrometheus(std::string& out) const override;

private:
	struct Series
	{
		std::vector<std::string> labelValues;
		double value = 0.0;
	};

	Series& GetOrCreateSeriesLocked(const std::vector<std::string>& labelValues);

	mutable std::mutex _lock;
	std::map<std::string, Series> _series;
};

class Gauge : public MetricBase
{
public:
	Gauge(const std::string& name, const std::string& help, const std::vector<std::string>& labelNames = {});

	void Set(double value, const MetricLabels& labels = {});
	void Add(double value, const MetricLabels& labels = {});
	void AppendPrometheus(std::string& out) const override;

private:
	struct Series
	{
		std::vector<std::string> labelValues;
		double value = 0.0;
	};

	Series& GetOrCreateSeriesLocked(const std::vector<std::string>& labelValues);

	mutable std::mutex _lock;
	std::map<std::string, Series> _series;
};

class Histogram : public MetricBase
{
public:
	Histogram(
		const std::string& name,
		const std::string& help,
		const std::vector<double>& buckets,
		const std::vector<std::string>& labelNames = {});

	void Observe(double valueInSeconds, const MetricLabels& labels = {});
	void AppendPrometheus(std::string& out) const override;

private:
	struct Series
	{
		std::vector<std::string> labelValues;
		std::vector<uint64_t> bucketCounts;
		uint64_t count = 0;
		double sum = 0.0;
	};

	Series& GetOrCreateSeriesLocked(const std::vector<std::string>& labelValues);

	std::vector<double> _buckets;
	mutable std::mutex _lock;
	std::map<std::string, Series> _series;
};

class MetricsRegistry
{
public:
	std::shared_ptr<Counter> RegisterCounter(
		const std::string& name,
		const std::string& help,
		const std::vector<std::string>& labelNames = {});

	std::shared_ptr<Gauge> RegisterGauge(
		const std::string& name,
		const std::string& help,
		const std::vector<std::string>& labelNames = {});

	std::shared_ptr<Histogram> RegisterHistogram(
		const std::string& name,
		const std::string& help,
		const std::vector<double>& buckets,
		const std::vector<std::string>& labelNames = {});

	std::vector<std::shared_ptr<MetricBase>> Snapshot() const;
	void Clear();

private:
	mutable std::mutex _lock;
	std::map<std::string, std::shared_ptr<MetricBase>> _metrics;
};
