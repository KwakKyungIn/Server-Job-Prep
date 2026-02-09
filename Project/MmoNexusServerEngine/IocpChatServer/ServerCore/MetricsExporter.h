#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>

#include "CoreGlobal.h"

class Histogram;
class MetricsRegistry;

class MetricsExporter
{
public:
	explicit MetricsExporter(MetricsRegistry& registry);
	~MetricsExporter();

	bool Start(
		const MetricsConfig& config,
		const std::function<void()>& beforeRenderCallback,
		const std::shared_ptr<Histogram>& responseTimeHistogram);
	void Stop();

	bool IsRunning() const;

private:
	void WorkerLoop();
	void HandleClient(SOCKET clientSocket);

	bool CreateListenSocket();
	void CloseListenSocket();

	bool SendAll(SOCKET socket, const std::string& payload);
	std::string BuildResponse(
		int statusCode,
		const std::string& statusText,
		const std::string& body,
		const std::string& contentType,
		const std::string& extraHeaders = "") const;

	bool ParseRequestLine(const std::string& request, std::string& method, std::string& path) const;

private:
	MetricsRegistry& _registry;
	MetricsConfig _config;
	std::function<void()> _beforeRenderCallback;
	std::shared_ptr<Histogram> _responseTimeHistogram;

	std::atomic<bool> _running{ false };
	std::atomic<SOCKET> _listenSocket{ INVALID_SOCKET };
	std::thread _worker;
};
