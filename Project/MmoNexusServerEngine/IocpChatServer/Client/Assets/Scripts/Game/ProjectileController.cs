using UnityEngine;

public class ProjectileController : MonoBehaviour
{
    Vector3 _targetPos;
    float _targetYaw;
    bool _hasTarget = false;

    // 테스트용: 너무 부드럽게 말고 “살짝만”
    const float LERP_POS = 0.65f;
    const float LERP_YAW = 0.65f;

    void Awake()
    {
        _targetPos = transform.position;
        _targetYaw = transform.eulerAngles.y;
    }

    public void SetTarget(Vector3 pos, float yaw)
    {
        _hasTarget = true;
        _targetPos = pos;
        _targetYaw = yaw;
    }

    void Update()
    {
        if (!_hasTarget) return;

        transform.position = Vector3.Lerp(transform.position, _targetPos, LERP_POS);

        float curYaw = transform.eulerAngles.y;
        float newYaw = Mathf.LerpAngle(curYaw, _targetYaw, LERP_YAW);
        transform.rotation = Quaternion.Euler(0f, newYaw, 0f);
    }
}
