#include "pch.h"
#include "PrometheusTextRenderer.h"

#include "Metrics.h"

std::string PrometheusTextRenderer::Render(const MetricsRegistry& registry)
{
	std::string result;
	const auto metrics = registry.Snapshot();

	for (size_t i = 0; i < metrics.size(); ++i)
	{
		metrics[i]->AppendPrometheus(result);
		if (i + 1 < metrics.size())
			result.push_back('\n');
	}

	return result;
}
