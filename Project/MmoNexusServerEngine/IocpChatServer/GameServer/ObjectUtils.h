#pragma once
#include "Protocol.pb.h"
#include <math.h>
#include <algorithm> // [New] min, max 등 안전장치용

#define PI 3.14159265f // [New] 원주율 정의

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

	// [New] 내적 (Dot Product) - 두 벡터의 방향 일치도 계산
	float Dot(const Vector3& other) const { return x * other.x + y * other.y + z * other.z; }
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

	// ==========================================================
	// [New] Hitbox Logic (기존 코드 영향 없음)
	// ==========================================================

	// 1. 원형 판정 (Circle)
	static bool CheckCircle(const Protocol::PositionInfo& center, float radius, const Protocol::PositionInfo& target)
	{
		float distSqr = DistSqr(center, target);
		return distSqr <= (radius * radius);
	}

	// 2. 부채꼴 판정 (Fan/Cone)
	// viewDir: 시선 방향 (정규화 필수)
	static bool CheckFan(const Protocol::PositionInfo& center, const Vector3& viewDir, float range, float angle, const Protocol::PositionInfo& target)
	{
		// 1) 거리 체크
		if (CheckCircle(center, range, target) == false)
			return false;

		// 2) 각도 체크 (내적)
		Vector3 dirToTarget = GetDirection(center, target);
		float dot = viewDir.Dot(dirToTarget); // 내적 계산

		// 부채꼴 각도의 절반보다 내적값이 크면(각도가 작으면) 히트
		float cosTheta = cosf((angle / 2.0f) * (PI / 180.0f));

		return dot >= cosTheta;
	}

	// [Helper] Yaw(회전값) -> 방향 벡터 변환
	static Vector3 GetVectorFromYaw(float yaw)
	{
		float rad = yaw * (PI / 180.0f);
		return Vector3(sinf(rad), 0, cosf(rad)); // Y축 회전 기준 (X, Z 평면)
	}

	// ObjectUtils.h
	static Vector3 GetDirectionXZ(const Protocol::PositionInfo& from, const Protocol::PositionInfo& to)
	{
		float dx = to.x() - from.x();
		float dz = to.z() - from.z();
		float len = sqrtf(dx * dx + dz * dz);
		if (len < 1e-6f) return Vector3(0, 0, 0);
		return Vector3(dx / len, 0.0f, dz / len);
	}

};