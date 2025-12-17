using UnityEngine;

public class AOIGridGizmos : MonoBehaviour
{
    // 서버 기준 고정값: (0~100), cellSize=10
    public int minX = 0;
    public int minY = 0;     // 서버 Y = 유니티 Z
    public int maxX = 100;
    public int maxY = 100;
    public int cellSize = 10;

    // 바닥이랑 겹쳐서 깜빡이는 것 방지
    public float drawY = 0.05f;

    // 전체 격자 vs 플레이어 주변만
    public bool drawFullGrid = true;
    public bool highlightAroundPlayer = true;

    // 9-grid 하이라이트 범위: 1 => 3x3
    public int rangeCells = 1;

    // 런타임에 자동으로 내 플레이어를 찾기 위한 설정
    public string myPlayerTag = "MyPlayer";
    Transform player;

    int GridSizeX => CeilDiv(maxX - minX, cellSize);
    int GridSizeY => CeilDiv(maxY - minY, cellSize);
    static int CeilDiv(int a, int b) => (a + b - 1) / b;

    void Update()
    {
        if (highlightAroundPlayer && player == null)
        {
            var go = GameObject.FindWithTag(myPlayerTag);
            if (go != null) player = go.transform;
        }
    }

    void OnDrawGizmos()
    {
        if (cellSize <= 0) return;
        if (maxX <= minX || maxY <= minY) return;

        int gsx = GridSizeX;
        int gsy = GridSizeY;

        Vector3 origin = new Vector3(minX, drawY, minY);

        // 1) 전체 그리드(고정 격자) - 회색
        if (drawFullGrid)
        {
            Gizmos.color = Color.gray; // [추가] 전체 그리드 색
            DrawGrid(origin, 0, gsx, 0, gsy);
        }

        // 2) 플레이어 주변 3x3 하이라이트 - 노랑 + 겹침 방지로 더 위에 그리기
        if (highlightAroundPlayer && player != null)
        {
            int px = Mathf.FloorToInt((player.position.x - minX) / cellSize);
            int py = Mathf.FloorToInt((player.position.z - minY) / cellSize);

            px = Mathf.Clamp(px, 0, gsx - 1);
            py = Mathf.Clamp(py, 0, gsy - 1);

            int sx = Mathf.Max(0, px - rangeCells);
            int ex = Mathf.Min(gsx, px + rangeCells + 1);
            int sy = Mathf.Max(0, py - rangeCells);
            int ey = Mathf.Min(gsy, py + rangeCells + 1);

            Gizmos.color = Color.yellow; // [추가] 하이라이트 색

            // [수정] 겹침 방지: drawY보다 더 띄워서 그림 (0.05f 정도 추천)
            Vector3 hiOrigin = new Vector3(origin.x, origin.y + 0.05f, origin.z);
            DrawGrid(hiOrigin, sx, ex, sy, ey);
        }

        Gizmos.color = Color.white; // [추가] 색 원복
    }

    void DrawGrid(Vector3 origin, int startX, int endX, int startY, int endY)
    {
        // 세로 라인
        for (int x = startX; x <= endX; x++)
        {
            float worldX = origin.x + x * cellSize;
            Vector3 from = new Vector3(worldX, origin.y, origin.z + startY * cellSize);
            Vector3 to = new Vector3(worldX, origin.y, origin.z + endY * cellSize);
            Gizmos.DrawLine(from, to);
        }

        // 가로 라인
        for (int y = startY; y <= endY; y++)
        {
            float worldZ = origin.z + y * cellSize;
            Vector3 from = new Vector3(origin.x + startX * cellSize, origin.y, worldZ);
            Vector3 to = new Vector3(origin.x + endX * cellSize, origin.y, worldZ);
            Gizmos.DrawLine(from, to);
        }
    }
}
