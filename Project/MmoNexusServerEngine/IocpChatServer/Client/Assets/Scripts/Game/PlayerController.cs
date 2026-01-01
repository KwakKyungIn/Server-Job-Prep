using UnityEngine;

public class PlayerController : MonoBehaviour
{
    Vector3 _targetPos;
    Quaternion _targetRot;

    float _moveSpeed = 5.0f;
    float _rotSpeed = 12.0f; // 회전 스무딩 속도(취향)

    void Start()
    {
        _targetPos = transform.position;
        _targetRot = transform.rotation;
    }

    void Update()
    {
        // =========================
        // Position Smoothing
        // =========================
        if (Vector3.Distance(transform.position, _targetPos) > 0.01f)
        {
            float step = _moveSpeed * Time.deltaTime;
            transform.position = Vector3.MoveTowards(transform.position, _targetPos, step);
        }

        // =========================
        // Rotation Smoothing
        // =========================
        // 목표 회전으로 부드럽게 따라가기
        transform.rotation = Quaternion.Slerp(transform.rotation, _targetRot, Time.deltaTime * _rotSpeed);
    }

    public void SetTargetPosition(Vector3 pos)
    {
        _targetPos = pos;
    }

    // ✅ 새로 추가: yaw만 받아서 회전 타겟 설정
    public void SetTargetYaw(float yaw)
    {
        _targetRot = Quaternion.Euler(0f, yaw, 0f);
    }
}
