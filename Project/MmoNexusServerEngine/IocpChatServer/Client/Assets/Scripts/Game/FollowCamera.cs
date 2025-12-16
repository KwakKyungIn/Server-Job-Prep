using UnityEngine;

public class FollowCamera : MonoBehaviour
{
    public Transform target;          // 따라갈 대상(내 플레이어)
    public Vector3 offset = new Vector3(0, 6, -8);
    public float followSpeed = 10f;

    void LateUpdate()
    {
        if (target == null) return;

        Vector3 desiredPos = target.position + offset;
        transform.position = Vector3.Lerp(transform.position, desiredPos, followSpeed * Time.deltaTime);

        transform.LookAt(target.position + Vector3.up * 1.5f);
    }
}
