#include "pch.h"
#include "Metrics.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <unordered_map>

namespace
{
	constexpr char kSeriesKeyDelimiter = '\x1F';

	std::string BuildTypeName(MetricType type)
	{
		switch (type)
		{
		case MetricType::Counter:
			return "counter";
		case MetricType::Gauge:
			return "gauge";
		case MetricType::Histogram:
			return "histogram";
		default:
			return "unknown";
		}
	}

	std::vector<double> SortedUniqueBuckets(const std::vector<double>& buckets)
	{
		std::vector<double> normalized = buckets;
		normalized.erase(
			std::remove_if(
				normalized.begin(),
				normalized.end(),
				[](double value)
				{
					return !(value >= 0.0);
				}),
			normalized.end());

		std::sort(normalized.begin(), normalized.end());
		normalized.erase(std::unique(normalized.begin(), normalized.end()), normalized.end());
		return normalized;
	}
}

double MetricsMillisecondsToSeconds(double milliseconds)
{
	return milliseconds / 1000.0;
}

namespace MetricsHistogramBuckets
{
	const std::vector<double>& PacketHandleSeconds()
	{
		static const std::vector<double> kBuckets = {
			MetricsMillisecondsToSeconds(0.001),
			MetricsMillisecondsToSeconds(0.005),
			MetricsMillisecondsToSeconds(0.01),
			MetricsMillisecondsToSeconds(0.05),
			MetricsMillisecondsToSeconds(0.1),
			MetricsMillisecondsToSeconds(0.5),
			MetricsMillisecondsToSeconds(1.0),
			MetricsMillisecondsToSeconds(5.0),
		};
		return kBuckets;
	}

	const std::vector<double>& DbQuerySeconds()
	{
		static const std::vector<double> kBuckets = {
			MetricsMillisecondsToSeconds(0.01),
			MetricsMillisecondsToSeconds(0.05),
			MetricsMillisecondsToSeconds(0.1),
			MetricsMillisecondsToSeconds(0.5),
			MetricsMillisecondsToSeconds(1.0),
			MetricsMillisecondsToSeconds(5.0),
			MetricsMillisecondsToSeconds(10.0),
		};
		return kBuckets;
	}

	const std::vector<double>& JobQueueWaitSeconds()
	{
		static const std::vector<double> kBuckets = {
			MetricsMillisecondsToSeconds(0.001),
			MetricsMillisecondsToSeconds(0.01),
			MetricsMillisecondsToSeconds(0.1),
			MetricsMillisecondsToSeconds(1.0),
			MetricsMillisecondsToSeconds(10.0),
			MetricsMillisecondsToSeconds(100.0),
		};
		return kBuckets;
	}
}

MetricBase::MetricBase(
	const std::string& name,
	const std::string& help,
	MetricType type,
	const std::vector<std::string>& labelNames)
	: _name(name),
	_help(help),
	_type(type),
	_labelNames(labelNames)
{
}

const std::string& MetricBase::GetName() const
{
	return _name;
}

const std::string& MetricBase::GetHelp() const
{
	return _help;
}

MetricType MetricBase::GetType() const
{
	return _type;
}

const std::vector<std::string>& MetricBase::GetLabelNames() const
{
	return _labelNames;
}

bool MetricBase::BuildOrderedLabelValues(const MetricLabels& labels, std::vector<std::string>& outValues) const
{
	if (_labelNames.empty())
	{
		if (labels.empty())
		{
			outValues.clear();
			return true;
		}
		return false;
	}

	if (labels.size() != _labelNames.size())
		return false;

	std::unordered_map<std::string, std::string> values;
	values.reserve(labels.size());

	for (const auto& label : labels)
	{
		auto inserted = values.emplace(label.first, label.second);
		if (inserted.second == false)
			return false;
	}

	outValues.clear();
	outValues.reserve(_labelNames.size());

	for (const std::string& labelName : _labelNames)
	{
		auto findIt = values.find(labelName);
		if (findIt == values.end())
			return false;

		outValues.push_back(findIt->second);
	}

	return true;
}

std::string MetricBase::BuildSeriesKey(const std::vector<std::string>& labelValues) const
{
	if (labelValues.empty())
		return std::string("_default");

	std::string key;
	for (size_t i = 0; i < labelValues.size(); ++i)
	{
		if (i > 0)
			key.push_back(kSeriesKeyDelimiter);
		key += labelValues[i];
	}
	return key;
}

void MetricBase::AppendLabelSet(
	std::string& out,
	const std::vector<std::string>& labelValues,
	const std::pair<std::string, std::string>* extraLabel) const
{
	if (labelValues.empty() && extraLabel == nullptr)
		return;

	out.push_back('{');
	bool needComma = false;

	for (size_t i = 0; i < _labelNames.size(); ++i)
	{
		if (needComma)
			out.push_back(',');

		out += _labelNames[i];
		out += "=\"";
		out += EscapeLabel(labelValues[i]);
		out += "\"";
		needComma = true;
	}

	if (extraLabel != nullptr)
	{
		if (needComma)
			out.push_back(',');

		out += extraLabel->first;
		out += "=\"";
		out += EscapeLabel(extraLabel->second);
		out += "\"";
	}

	out.push_back('}');
}

std::string MetricBase::EscapeHelp(const std::string& value)
{
	std::string escaped;
	escaped.reserve(value.size());

	for (char ch : value)
	{
		switch (ch)
		{
		case '\\':
			escaped += "\\\\";
			break;
		case '\n':
			escaped += "\\n";
			break;
		default:
			escaped.push_back(ch);
			break;
		}
	}

	return escaped;
}

std::string MetricBase::EscapeLabel(const std::string& value)
{
	std::string escaped;
	escaped.reserve(value.size());

	for (char ch : value)
	{
		switch (ch)
		{
		case '\\':
			escaped += "\\\\";
			break;
		case '"':
			escaped += "\\\"";
			break;
		case '\n':
			escaped += "\\n";
			break;
		default:
			escaped.push_back(ch);
			break;
		}
	}

	return escaped;
}

std::string MetricBase::FormatDouble(double value)
{
	if (std::isnan(value))
		return "NaN";
	if (std::isinf(value))
		return value > 0.0 ? "+Inf" : "-Inf";

	std::ostringstream oss;
	oss << std::setprecision(15) << value;
	return oss.str();
}

Counter::Counter(const std::string& name, const std::string& help, const std::vector<std::string>& labelNames)
	: MetricBase(name, help, MetricType::Counter, labelNames)
{
	if (GetLabelNames().empty())
	{
		std::vector<std::string> noLabels;
		std::lock_guard<std::mutex> guard(_lock);
		_series.emplace(BuildSeriesKey(noLabels), Series{ noLabels, 0.0 });
	}
}

Counter::Series& Counter::GetOrCreateSeriesLocked(const std::vector<std::string>& labelValues)
{
	const std::string key = BuildSeriesKey(labelValues);
	auto findIt = _series.find(key);
	if (findIt != _series.end())
		return findIt->second;

	auto inserted = _series.emplace(key, Series{ labelValues, 0.0 });
	return inserted.first->second;
}

void Counter::Inc(double value, const MetricLabels& labels)
{
	if (value < 0.0)
		return;

	std::vector<std::string> orderedValues;
	if (BuildOrderedLabelValues(labels, orderedValues) == false)
		return;

	std::lock_guard<std::mutex> guard(_lock);
	Series& series = GetOrCreateSeriesLocked(orderedValues);
	series.value += value;
}

void Counter::AppendPrometheus(std::string& out) const
{
	out += "# HELP ";
	out += GetName();
	out.push_back(' ');
	out += EscapeHelp(GetHelp());
	out.push_back('\n');

	out += "# TYPE ";
	out += GetName();
	out.push_back(' ');
	out += BuildTypeName(GetType());
	out.push_back('\n');

	std::lock_guard<std::mutex> guard(_lock);
	for (const auto& pair : _series)
	{
		const Series& series = pair.second;
		out += GetName();
		AppendLabelSet(out, series.labelValues);
		out.push_back(' ');
		out += FormatDouble(series.value);
		out.push_back('\n');
	}
}

Gauge::Gauge(const std::string& name, const std::string& help, const std::vector<std::string>& labelNames)
	: MetricBase(name, help, MetricType::Gauge, labelNames)
{
	if (GetLabelNames().empty())
	{
		std::vector<std::string> noLabels;
		std::lock_guard<std::mutex> guard(_lock);
		_series.emplace(BuildSeriesKey(noLabels), Series{ noLabels, 0.0 });
	}
}

Gauge::Series& Gauge::GetOrCreateSeriesLocked(const std::vector<std::string>& labelValues)
{
	const std::string key = BuildSeriesKey(labelValues);
	auto findIt = _series.find(key);
	if (findIt != _series.end())
		return findIt->second;

	auto inserted = _series.emplace(key, Series{ labelValues, 0.0 });
	return inserted.first->second;
}

void Gauge::Set(double value, const MetricLabels& labels)
{
	std::vector<std::string> orderedValues;
	if (BuildOrderedLabelValues(labels, orderedValues) == false)
		return;

	std::lock_guard<std::mutex> guard(_lock);
	Series& series = GetOrCreateSeriesLocked(orderedValues);
	series.value = value;
}

void Gauge::Add(double value, const MetricLabels& labels)
{
	std::vector<std::string> orderedValues;
	if (BuildOrderedLabelValues(labels, orderedValues) == false)
		return;

	std::lock_guard<std::mutex> guard(_lock);
	Series& series = GetOrCreateSeriesLocked(orderedValues);
	series.value += value;
}

void Gauge::AppendPrometheus(std::string& out) const
{
	out += "# HELP ";
	out += GetName();
	out.push_back(' ');
	out += EscapeHelp(GetHelp());
	out.push_back('\n');

	out += "# TYPE ";
	out += GetName();
	out.push_back(' ');
	out += BuildTypeName(GetType());
	out.push_back('\n');

	std::lock_guard<std::mutex> guard(_lock);
	for (const auto& pair : _series)
	{
		const Series& series = pair.second;
		out += GetName();
		AppendLabelSet(out, series.labelValues);
		out.push_back(' ');
		out += FormatDouble(series.value);
		out.push_back('\n');
	}
}

Histogram::Histogram(
	const std::string& name,
	const std::string& help,
	const std::vector<double>& buckets,
	const std::vector<std::string>& labelNames)
	: MetricBase(name, help, MetricType::Histogram, labelNames),
	_buckets(SortedUniqueBuckets(buckets))
{
	if (GetLabelNames().empty())
	{
		std::vector<std::string> noLabels;
		std::lock_guard<std::mutex> guard(_lock);
		Series series;
		series.labelValues = noLabels;
		series.bucketCounts.assign(_buckets.size(), 0);
		_series.emplace(BuildSeriesKey(noLabels), std::move(series));
	}
}

Histogram::Series& Histogram::GetOrCreateSeriesLocked(const std::vector<std::string>& labelValues)
{
	const std::string key = BuildSeriesKey(labelValues);
	auto findIt = _series.find(key);
	if (findIt != _series.end())
		return findIt->second;

	Series series;
	series.labelValues = labelValues;
	series.bucketCounts.assign(_buckets.size(), 0);
	auto inserted = _series.emplace(key, std::move(series));
	return inserted.first->second;
}

void Histogram::Observe(double valueInSeconds, const MetricLabels& labels)
{
	if (valueInSeconds < 0.0)
		valueInSeconds = 0.0;

	std::vector<std::string> orderedValues;
	if (BuildOrderedLabelValues(labels, orderedValues) == false)
		return;

	std::lock_guard<std::mutex> guard(_lock);
	Series& series = GetOrCreateSeriesLocked(orderedValues);
	series.sum += valueInSeconds;
	series.count += 1;

	if (_buckets.empty())
		return;

	auto bucketIt = std::lower_bound(_buckets.begin(), _buckets.end(), valueInSeconds);
	if (bucketIt != _buckets.end())
	{
		const size_t index = static_cast<size_t>(bucketIt - _buckets.begin());
		series.bucketCounts[index] += 1;
	}
}

void Histogram::AppendPrometheus(std::string& out) const
{
	out += "# HELP ";
	out += GetName();
	out.push_back(' ');
	out += EscapeHelp(GetHelp());
	out.push_back('\n');

	out += "# TYPE ";
	out += GetName();
	out.push_back(' ');
	out += BuildTypeName(GetType());
	out.push_back('\n');

	std::lock_guard<std::mutex> guard(_lock);
	for (const auto& pair : _series)
	{
		const Series& series = pair.second;
		uint64_t cumulative = 0;

		for (size_t bucketIndex = 0; bucketIndex < _buckets.size(); ++bucketIndex)
		{
			cumulative += series.bucketCounts[bucketIndex];

			out += GetName();
			out += "_bucket";
			const std::pair<std::string, std::string> leLabel =
			{
				"le",
				FormatDouble(_buckets[bucketIndex])
			};
			AppendLabelSet(out, series.labelValues, &leLabel);
			out.push_back(' ');
			out += std::to_string(cumulative);
			out.push_back('\n');
		}

		out += GetName();
		out += "_bucket";
		const std::pair<std::string, std::string> infLabel = { "le", "+Inf" };
		AppendLabelSet(out, series.labelValues, &infLabel);
		out.push_back(' ');
		out += std::to_string(series.count);
		out.push_back('\n');

		out += GetName();
		out += "_sum";
		AppendLabelSet(out, series.labelValues);
		out.push_back(' ');
		out += FormatDouble(series.sum);
		out.push_back('\n');

		out += GetName();
		out += "_count";
		AppendLabelSet(out, series.labelValues);
		out.push_back(' ');
		out += std::to_string(series.count);
		out.push_back('\n');
	}
}

std::shared_ptr<Counter> MetricsRegistry::RegisterCounter(
	const std::string& name,
	const std::string& help,
	const std::vector<std::string>& labelNames)
{
	std::lock_guard<std::mutex> guard(_lock);
	auto findIt = _metrics.find(name);
	if (findIt != _metrics.end())
		return std::dynamic_pointer_cast<Counter>(findIt->second);

	std::shared_ptr<Counter> metric(new Counter(name, help, labelNames));
	_metrics.emplace(name, metric);
	return metric;
}

std::shared_ptr<Gauge> MetricsRegistry::RegisterGauge(
	const std::string& name,
	const std::string& help,
	const std::vector<std::string>& labelNames)
{
	std::lock_guard<std::mutex> guard(_lock);
	auto findIt = _metrics.find(name);
	if (findIt != _metrics.end())
		return std::dynamic_pointer_cast<Gauge>(findIt->second);

	std::shared_ptr<Gauge> metric(new Gauge(name, help, labelNames));
	_metrics.emplace(name, metric);
	return metric;
}

std::shared_ptr<Histogram> MetricsRegistry::RegisterHistogram(
	const std::string& name,
	const std::string& help,
	const std::vector<double>& buckets,
	const std::vector<std::string>& labelNames)
{
	std::lock_guard<std::mutex> guard(_lock);
	auto findIt = _metrics.find(name);
	if (findIt != _metrics.end())
		return std::dynamic_pointer_cast<Histogram>(findIt->second);

	std::shared_ptr<Histogram> metric(new Histogram(name, help, buckets, labelNames));
	_metrics.emplace(name, metric);
	return metric;
}

std::vector<std::shared_ptr<MetricBase>> MetricsRegistry::Snapshot() const
{
	std::vector<std::shared_ptr<MetricBase>> metrics;

	std::lock_guard<std::mutex> guard(_lock);
	metrics.reserve(_metrics.size());
	for (const auto& pair : _metrics)
		metrics.push_back(pair.second);

	return metrics;
}

void MetricsRegistry::Clear()
{
	std::lock_guard<std::mutex> guard(_lock);
	_metrics.clear();
}
