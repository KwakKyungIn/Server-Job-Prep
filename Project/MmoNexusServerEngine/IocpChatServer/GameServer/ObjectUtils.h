#pragma once
#include "Protocol.pb.h"
#include <math.h>

struct Vector3
{
	float x, y, z;

	Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
	Vector3(const Protocol::PositionInfo& info) : x(info.x()), y(info.y()), z(info.z()) {}

	// 벡터 뺄셈
	Vector3 operator-(const Vector3& other)
	{
		return Vector3(x - other.x, y - other.y, z - other.z);
	}

	// 벡터 덧셈
	Vector3 operator+(const Vector3& other)
	{
		return Vector3(x + other.x, y + other.y, z + other.z);
	}

	// 스칼라 곱셈
	Vector3 operator*(float scalar)
	{
		return Vector3(x * scalar, y * scalar, z * scalar);
	}

	// 길이 (Magnitude)
	float Length() { return sqrt(x * x + y * y + z * z); }

	// 길이 제곱 (최적화용)
	float LengthSquared() { return x * x + y * y + z * z; }

	// 정규화 (방향만 남김)
	void Normalize()
	{
		float len = Length();
		if (len < 0.0001f) return; // 0으로 나누기 방지
		x /= len;
		y /= len;
		z /= len;
	}
};

class ObjectUtils
{
public:
	// 두 위치 사이의 거리 제곱 (루트 연산 없음 -> 빠름)
	static float DistSqr(const Protocol::PositionInfo& a, const Protocol::PositionInfo& b)
	{
		float dx = a.x() - b.x();
		float dy = a.z() - b.z(); // MMO는 보통 X, Z 평면 이동
		return dx * dx + dy * dy;
	}

	// 두 위치 사이의 거리 (루트 연산 있음 -> 느림)
	static float Dist(const Protocol::PositionInfo& a, const Protocol::PositionInfo& b)
	{
		return sqrt(DistSqr(a, b));
	}

	// a가 b를 바라보는 방향 벡터 반환
	static Vector3 GetDirection(const Protocol::PositionInfo& from, const Protocol::PositionInfo& to)
	{
		Vector3 vFrom(from);
		Vector3 vTo(to);
		Vector3 dir = vTo - vFrom;
		dir.Normalize();
		return dir;
	}
};