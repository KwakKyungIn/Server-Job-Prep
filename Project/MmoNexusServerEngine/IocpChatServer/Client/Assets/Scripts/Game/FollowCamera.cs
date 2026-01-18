using UnityEngine;
using UnityEngine.EventSystems; // ✅ UI 위 입력 차단

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

    [Header("Input")]
    [Tooltip("Right click is treated as orbit only when the mouse is dragged more than this many pixels.")]
    public float rightDragThresholdPx = 6f;

    float _yaw;
    float _pitch;

    bool _orbiting = false;
    Vector3 _rmbDownPos;

    void Start()
    {
        if (target == null) return;

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

        // ✅ 핵심: 마우스가 UI 위면 카메라 입력을 먹지 않는다
        bool overUI = (EventSystem.current != null) && EventSystem.current.IsPointerOverGameObject();
        if (overUI)
        {
            ResetOrbitInput();
            SetCursorUnlocked();
        }
        else
        {
            // ✅ [FIX] RMB 클릭(월드 상호작용) vs RMB 드래그(카메라 회전) 분리
            if (Input.GetMouseButtonDown(1))
            {
                _orbiting = false;
                _rmbDownPos = Input.mousePosition;
            }

            if (Input.GetMouseButton(1))
            {
                // 드래그 임계치 넘기기 전에는 '클릭'으로 취급해서 커서 락/회전 금지
                if (_orbiting == false)
                {
                    float drag = (Input.mousePosition - _rmbDownPos).magnitude;
                    if (drag >= rightDragThresholdPx)
                        _orbiting = true;
                }

                if (_orbiting)
                {
                    _yaw += Input.GetAxis("Mouse X") * mouseSensitivity;
                    _pitch -= Input.GetAxis("Mouse Y") * mouseSensitivity;
                    _pitch = ClampAngle(_pitch, pitchMin, pitchMax);

                    Cursor.lockState = CursorLockMode.Locked;
                    Cursor.visible = false;
                }
                else
                {
                    SetCursorUnlocked();
                }
            }
            else
            {
                ResetOrbitInput();
                SetCursorUnlocked();
            }

            if (Input.GetMouseButtonUp(1))
            {
                ResetOrbitInput();
                SetCursorUnlocked();
            }
        }

        Quaternion rot = Quaternion.Euler(_pitch, _yaw, 0f);

        Vector3 desiredPos = target.position + rot * new Vector3(0f, height, -distance);
        transform.position = Vector3.Lerp(transform.position, desiredPos, followSpeed * Time.deltaTime);

        Quaternion desiredRot = Quaternion.LookRotation((target.position + Vector3.up * 1.5f) - transform.position);
        transform.rotation = Quaternion.Slerp(transform.rotation, desiredRot, rotateSpeed * Time.deltaTime);
    }

    void ResetOrbitInput()
    {
        _orbiting = false;
    }

    void SetCursorUnlocked()
    {
        Cursor.lockState = CursorLockMode.None;
        Cursor.visible = true;
    }

    static float ClampAngle(float angle, float min, float max)
    {
        while (angle > 180f) angle -= 360f;
        while (angle < -180f) angle += 360f;
        return Mathf.Clamp(angle, min, max);
    }

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
