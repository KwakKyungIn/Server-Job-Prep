using UnityEngine;

public class PlayerController : MonoBehaviour
{
    Vector3 _targetPos;
    float _speed = 5.0f;

    void Start()
    {
        _targetPos = transform.position;
    }

    void Update()
    {
        // [Smoothing] 현재 위치에서 목표 위치로 부드럽게 이동 (Lerp)
        // 실제 게임에서는 Dead Reckoning 등 더 복잡한 알고리즘을 쓰지만, 일단 이걸로 충분함.
        if (Vector3.Distance(transform.position, _targetPos) > 0.01f)
        {
            float step = _speed * Time.deltaTime;
            transform.position = Vector3.MoveTowards(transform.position, _targetPos, step);
        }
    }

    public void SetTargetPosition(Vector3 pos)
    {
        _targetPos = pos;
    }
}