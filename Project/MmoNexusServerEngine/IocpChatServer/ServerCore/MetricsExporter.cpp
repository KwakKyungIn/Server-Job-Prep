#include "pch.h"
#include "MetricsExporter.h"

#include "Metrics.h"
#include "PrometheusTextRenderer.h"

#include <climits>
#include <chrono>
#include <sstream>

namespace
{
	std::string NormalizePath(const std::string& path)
	{
		if (path.empty())
			return "/metrics";

		if (path.front() == '/')
			return path;

		return "/" + path;
	}
}

MetricsExporter::MetricsExporter(MetricsRegistry& registry)
	: _registry(registry)
{
}

MetricsExporter::~MetricsExporter()
{
	Stop();
}

bool MetricsExporter::Start(
	const MetricsConfig& config,
	const std::function<void()>& beforeRenderCallback,
	const std::shared_ptr<Histogram>& responseTimeHistogram)
{
	if (_running.load())
		return true;

	if (config.Port <= 0 || config.Port > 65535)
	{
		std::cout << "[Metrics][WARN] Invalid exporter port: " << config.Port << std::endl;
		return false;
	}

	_config = config;
	_config.Path = NormalizePath(config.Path);
	if (_config.BindAddress.empty())
		_config.BindAddress = "127.0.0.1";

	_beforeRenderCallback = beforeRenderCallback;
	_responseTimeHistogram = responseTimeHistogram;

	if (CreateListenSocket() == false)
		return false;

	_running.store(true);
	_worker = std::thread([this]()
		{
			WorkerLoop();
		});

	return true;
}

void MetricsExporter::Stop()
{
	if (_running.exchange(false) == false)
		return;

	CloseListenSocket();

	if (_worker.joinable())
		_worker.join();
}

bool MetricsExporter::IsRunning() const
{
	return _running.load();
}

void MetricsExporter::WorkerLoop()
{
	while (_running.load() && GIsRunning.load())
	{
		const SOCKET listenSocket = _listenSocket.load();
		if (listenSocket == INVALID_SOCKET)
			break;

		fd_set readSet;
		FD_ZERO(&readSet);
		FD_SET(listenSocket, &readSet);

		timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		const int selectResult = ::select(0, &readSet, nullptr, nullptr, &timeout);
		if (selectResult == 0)
			continue;

		if (selectResult == SOCKET_ERROR)
		{
			const int errorCode = ::WSAGetLastError();
			if (_running.load() == false || GIsRunning.load() == false)
				break;

			std::cout << "[Metrics][WARN] select() failed: " << errorCode << std::endl;
			continue;
		}

		SOCKET clientSocket = ::accept(listenSocket, nullptr, nullptr);
		if (clientSocket == INVALID_SOCKET)
		{
			const int errorCode = ::WSAGetLastError();
			if (_running.load() == false || GIsRunning.load() == false)
				break;

			std::cout << "[Metrics][WARN] accept() failed: " << errorCode << std::endl;
			continue;
		}

		HandleClient(clientSocket);
		::closesocket(clientSocket);
	}
}

void MetricsExporter::HandleClient(SOCKET clientSocket)
{
	char recvBuffer[8192] = {};
	const int receivedBytes = ::recv(clientSocket, recvBuffer, static_cast<int>(sizeof(recvBuffer) - 1), 0);
	if (receivedBytes <= 0)
		return;

	std::string request(recvBuffer, static_cast<size_t>(receivedBytes));
	std::string method;
	std::string path;
	if (ParseRequestLine(request, method, path) == false)
	{
		const std::string response = BuildResponse(400, "Bad Request", "bad request\n", "text/plain; charset=utf-8");
		SendAll(clientSocket, response);
		return;
	}

	const size_t queryPos = path.find('?');
	if (queryPos != std::string::npos)
		path = path.substr(0, queryPos);

	if (method != "GET")
	{
		const std::string response = BuildResponse(
			405,
			"Method Not Allowed",
			"method not allowed\n",
			"text/plain; charset=utf-8",
			"Allow: GET\r\n");
		SendAll(clientSocket, response);
		return;
	}

	if (path != _config.Path)
	{
		const std::string response = BuildResponse(404, "Not Found", "not found\n", "text/plain; charset=utf-8");
		SendAll(clientSocket, response);
		return;
	}

	if (_beforeRenderCallback)
		_beforeRenderCallback();

	const auto begin = std::chrono::steady_clock::now();
	const std::string metricsBody = PrometheusTextRenderer::Render(_registry);
	const auto end = std::chrono::steady_clock::now();
	const double elapsedSeconds = std::chrono::duration<double>(end - begin).count();

	if (_responseTimeHistogram)
		_responseTimeHistogram->Observe(elapsedSeconds);

	if (elapsedSeconds > MetricsMillisecondsToSeconds(500.0))
	{
		std::cout << "[Metrics][WARN] /metrics render time exceeded 500ms: "
			<< (elapsedSeconds * 1000.0) << "ms" << std::endl;
	}
	else if (elapsedSeconds > MetricsMillisecondsToSeconds(100.0))
	{
		std::cout << "[Metrics][INFO] /metrics render time exceeded target(100ms): "
			<< (elapsedSeconds * 1000.0) << "ms" << std::endl;
	}

	const std::string response = BuildResponse(
		200,
		"OK",
		metricsBody,
		"text/plain; version=0.0.4; charset=utf-8");
	SendAll(clientSocket, response);
}

bool MetricsExporter::CreateListenSocket()
{
	SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET)
	{
		std::cout << "[Metrics][WARN] socket() failed: " << ::WSAGetLastError() << std::endl;
		return false;
	}

	BOOL reuseAddress = TRUE;
	::setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuseAddress), sizeof(reuseAddress));

	sockaddr_in bindAddress;
	::ZeroMemory(&bindAddress, sizeof(bindAddress));
	bindAddress.sin_family = AF_INET;
	bindAddress.sin_port = ::htons(static_cast<u_short>(_config.Port));

	std::string bindAddressText = _config.BindAddress;
	if (::inet_pton(AF_INET, bindAddressText.c_str(), &bindAddress.sin_addr) != 1)
	{
		std::cout << "[Metrics][WARN] Invalid bind address: " << bindAddressText
			<< ", fallback to 127.0.0.1" << std::endl;
		bindAddressText = "127.0.0.1";
		if (::inet_pton(AF_INET, bindAddressText.c_str(), &bindAddress.sin_addr) != 1)
		{
			::closesocket(listenSocket);
			return false;
		}
	}

	if (::bind(listenSocket, reinterpret_cast<const sockaddr*>(&bindAddress), sizeof(bindAddress)) == SOCKET_ERROR)
	{
		std::cout << "[Metrics][WARN] bind() failed on " << bindAddressText << ":" << _config.Port
			<< " error=" << ::WSAGetLastError() << std::endl;
		::closesocket(listenSocket);
		return false;
	}

	if (::listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		std::cout << "[Metrics][WARN] listen() failed: " << ::WSAGetLastError() << std::endl;
		::closesocket(listenSocket);
		return false;
	}

	_listenSocket.store(listenSocket);
	std::cout << "[Metrics] Exporter listening on http://"
		<< bindAddressText << ":" << _config.Port << _config.Path << std::endl;
	return true;
}

void MetricsExporter::CloseListenSocket()
{
	const SOCKET socketToClose = _listenSocket.exchange(INVALID_SOCKET);
	if (socketToClose == INVALID_SOCKET)
		return;

	::shutdown(socketToClose, SD_BOTH);
	::closesocket(socketToClose);
}

bool MetricsExporter::SendAll(SOCKET socket, const std::string& payload)
{
	size_t sentBytes = 0;
	while (sentBytes < payload.size())
	{
		const size_t remaining = payload.size() - sentBytes;
		const size_t maxChunk = static_cast<size_t>(INT_MAX);
		const size_t chunk = (remaining > maxChunk) ? maxChunk : remaining;
		const int sent = ::send(
			socket,
			payload.data() + sentBytes,
			static_cast<int>(chunk),
			0);

		if (sent == SOCKET_ERROR || sent == 0)
			return false;

		sentBytes += static_cast<size_t>(sent);
	}

	return true;
}

std::string MetricsExporter::BuildResponse(
	int statusCode,
	const std::string& statusText,
	const std::string& body,
	const std::string& contentType,
	const std::string& extraHeaders) const
{
	std::ostringstream oss;
	oss << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
	oss << "Content-Type: " << contentType << "\r\n";
	oss << "Content-Length: " << body.size() << "\r\n";
	oss << "Connection: close\r\n";
	if (extraHeaders.empty() == false)
		oss << extraHeaders;
	oss << "\r\n";
	oss << body;
	return oss.str();
}

bool MetricsExporter::ParseRequestLine(const std::string& request, std::string& method, std::string& path) const
{
	const size_t lineEnd = request.find("\r\n");
	if (lineEnd == std::string::npos)
		return false;

	const std::string requestLine = request.substr(0, lineEnd);
	std::istringstream iss(requestLine);
	std::string version;
	iss >> method >> path >> version;
	if (method.empty() || path.empty() || version.empty())
		return false;

	return true;
}
