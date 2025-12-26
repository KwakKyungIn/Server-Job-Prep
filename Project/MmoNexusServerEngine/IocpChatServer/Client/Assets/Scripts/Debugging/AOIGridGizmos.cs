using UnityEngine;

[ExecuteAlways]
[DisallowMultipleComponent]
public class AOIGridGizmos : MonoBehaviour
{
    // 서버 좌표계: x (0~sizeX), y (0~sizeY)
    // 유니티 좌표계: x, z 를 사용
    [Header("Server Map Bounds (Unity X/Z)")]
    public int minX = 0;
    public int minZ = 0;  // 서버 y == 유니티 z
    public int maxX = 1000;
    public int maxZ = 1000;

    [Header("Cell Size")]
    public int cellSize = 100;

    [Header("Draw Options")]
    public float drawY = 0.05f;
    public bool drawFullGrid = true;
    public bool highlightAroundPlayer = true;
    public int rangeCells = 1; // 1 => 3x3
    public string myPlayerTag = "MyPlayer";

    Transform _player;

    int GridSizeX => CeilDiv(maxX - minX, cellSize);
    int GridSizeZ => CeilDiv(maxZ - minZ, cellSize);
    static int CeilDiv(int a, int b) => (a + b - 1) / b;

    void OnValidate()
    {
        if (cellSize < 1) cellSize = 1;
        if (maxX <= minX) maxX = minX + 1;
        if (maxZ <= minZ) maxZ = minZ + 1;
        if (rangeCells < 0) rangeCells = 0;
        if (drawY < 0f) drawY = 0f;
    }

    void Update()
    {
        // 에디터에서도 돌아가니까 Find는 "필요할 때만"
        if (!highlightAroundPlayer) return;
        if (_player != null) return;

        if (Application.isPlaying)
        {
            var go = GameObject.FindWithTag(myPlayerTag);
            if (go != null) _player = go.transform;
        }
    }

    void OnDrawGizmos()
    {
        if (cellSize <= 0) return;
        if (maxX <= minX || maxZ <= minZ) return;

        int gsx = GridSizeX;
        int gsz = GridSizeZ;

        Vector3 origin = new Vector3(minX, drawY, minZ);

        // 1) 전체 그리드
        if (drawFullGrid)
        {
            Gizmos.color = Color.gray;
            DrawGrid(origin, 0, gsx, 0, gsz);
        }

        // 2) 내 주변 3x3 하이라이트
        if (highlightAroundPlayer)
        {
            Transform p = _player;

            // 플레이 중 아니면, Scene에서라도 태그 찾아보게
            if (p == null)
            {
                var go = GameObject.FindWithTag(myPlayerTag);
                if (go != null) p = go.transform;
            }

            if (p != null)
            {
                int px = Mathf.FloorToInt((p.position.x - minX) / (float)cellSize);
                int pz = Mathf.FloorToInt((p.position.z - minZ) / (float)cellSize);

                px = Mathf.Clamp(px, 0, gsx - 1);
                pz = Mathf.Clamp(pz, 0, gsz - 1);

                int sx = Mathf.Max(0, px - rangeCells);
                int ex = Mathf.Min(gsx, px + rangeCells + 1);
                int sz = Mathf.Max(0, pz - rangeCells);
                int ez = Mathf.Min(gsz, pz + rangeCells + 1);

                Gizmos.color = Color.yellow;

                // Z-fighting 방지로 살짝 더 띄움
                Vector3 hiOrigin = new Vector3(origin.x, origin.y + 0.05f, origin.z);
                DrawGrid(hiOrigin, sx, ex, sz, ez);
            }
        }

        Gizmos.color = Color.white;
    }

    void DrawGrid(Vector3 origin, int startX, int endX, int startZ, int endZ)
    {
        // 세로 라인 (X)
        for (int x = startX; x <= endX; x++)
        {
            float worldX = origin.x + x * cellSize;
            Vector3 from = new Vector3(worldX, origin.y, origin.z + startZ * cellSize);
            Vector3 to = new Vector3(worldX, origin.y, origin.z + endZ * cellSize);
            Gizmos.DrawLine(from, to);
        }

        // 가로 라인 (Z)
        for (int z = startZ; z <= endZ; z++)
        {
            float worldZ = origin.z + z * cellSize;
            Vector3 from = new Vector3(origin.x + startX * cellSize, origin.y, worldZ);
            Vector3 to = new Vector3(origin.x + endX * cellSize, origin.y, worldZ);
            Gizmos.DrawLine(from, to);
        }
    }
}
