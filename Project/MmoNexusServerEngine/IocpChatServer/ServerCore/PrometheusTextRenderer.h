#pragma once

#include <string>

class MetricsRegistry;

class PrometheusTextRenderer
{
public:
	static std::string Render(const MetricsRegistry& registry);
};
