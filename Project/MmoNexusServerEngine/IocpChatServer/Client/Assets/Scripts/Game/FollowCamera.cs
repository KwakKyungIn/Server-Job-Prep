using UnityEngine;

public class FollowCamera : MonoBehaviour
{
    public Transform target;

    [Header("Distance / Height")]
    public float distance = 8f;
    public float height = 3f;

    [Header("Orbit")]
    public float mouseSensitivity = 3f;
    public float pitchMin = -20f;
    public float pitchMax = 70f;

    [Header("Smoothing")]
    public float followSpeed = 12f;
    public float rotateSpeed = 12f;

    float _yaw;
    float _pitch;

    void Start()
    {
        if (target == null) return;

        // 현재 카메라 방향에서 초기 yaw/pitch 추정
        Vector3 dir = (transform.position - target.position);
        if (dir.sqrMagnitude < 0.001f) dir = new Vector3(0, height, -distance);

        Quaternion look = Quaternion.LookRotation(dir.normalized);
        Vector3 e = look.eulerAngles;
        _yaw = e.y;
        _pitch = e.x;
    }

    void LateUpdate()
    {
        if (target == null) return;

        // 우클릭 누르는 동안만 회전 (MMO 기본)
        if (Input.GetMouseButton(1))
        {
            _yaw += Input.GetAxis("Mouse X") * mouseSensitivity;
            _pitch -= Input.GetAxis("Mouse Y") * mouseSensitivity;
            _pitch = ClampAngle(_pitch, pitchMin, pitchMax);

            // 마우스 회전 중 커서 고정은 옵션
            Cursor.lockState = CursorLockMode.Locked;
            Cursor.visible = false;
        }
        else
        {
            Cursor.lockState = CursorLockMode.None;
            Cursor.visible = true;
        }

        Quaternion rot = Quaternion.Euler(_pitch, _yaw, 0f);

        // target 기준으로 "뒤로 distance" + 위로 height
        Vector3 desiredPos = target.position + rot * new Vector3(0f, height, -distance);

        transform.position = Vector3.Lerp(transform.position, desiredPos, followSpeed * Time.deltaTime);

        // 시선도 부드럽게
        Quaternion desiredRot = Quaternion.LookRotation((target.position + Vector3.up * 1.5f) - transform.position);
        transform.rotation = Quaternion.Slerp(transform.rotation, desiredRot, rotateSpeed * Time.deltaTime);
    }

    static float ClampAngle(float angle, float min, float max)
    {
        // 0~360 보정
        while (angle > 180f) angle -= 360f;
        while (angle < -180f) angle += 360f;
        return Mathf.Clamp(angle, min, max);
    }

    // 플레이어가 카메라 기준 이동하려면 "카메라 forward/right"가 필요
    public Vector3 GetPlanarForward()
    {
        Vector3 f = transform.forward;
        f.y = 0f;
        return f.sqrMagnitude < 0.001f ? Vector3.forward : f.normalized;
    }

    public Vector3 GetPlanarRight()
    {
        Vector3 r = transform.right;
        r.y = 0f;
        return r.sqrMagnitude < 0.001f ? Vector3.right : r.normalized;
    }
}
