using UnityEngine;
using UnityEditor;
using System.IO;
using System.Text;
using System.Globalization;

public class MapExporter
{
    [MenuItem("GigaChad/Export Map to OBJ")]
    static void ExportWholeMap()
    {
        if (Selection.gameObjects.Length == 0)
        {
            EditorUtility.DisplayDialog("경고", "맵의 최상위 부모 오브젝트를 선택해라, 곽삣삐.", "OK");
            return;
        }

        GameObject root = Selection.gameObjects[0];
        string path = EditorUtility.SaveFilePanel("Save Map .obj", "", root.name + "_NavGeo", "obj");
        if (string.IsNullOrEmpty(path)) return;

        MeshFilter[] meshFilters = root.GetComponentsInChildren<MeshFilter>(true);

        // 옵션: Unity -> Recast 좌표계 변환이 필요하면 true
        // (필요 없으면 false로 두고, 나중에 네 베이커/로더에서 축 처리해도 됨)
        bool flipX = false;

        var inv = CultureInfo.InvariantCulture;

        int vertexOffset = 0;

        using (var sw = new StreamWriter(path, false, Encoding.UTF8))
        {
            sw.WriteLine("# GigaChad Map Exporter v1.1");

            foreach (MeshFilter mf in meshFilters)
            {
                Mesh mesh = mf.sharedMesh;
                if (!mesh) continue;

                // 비활성/렌더 끈 오브젝트 빼고 싶으면 아래 체크 추가 가능
                // var r = mf.GetComponent<MeshRenderer>();
                // if (r == null || !r.enabled) continue;

                Matrix4x4 matrix = mf.transform.localToWorldMatrix;

                // 1) Vertices
                var verts = mesh.vertices;
                for (int i = 0; i < verts.Length; i++)
                {
                    Vector3 wp = matrix.MultiplyPoint3x4(verts[i]);
                    float x = flipX ? -wp.x : wp.x;

                    sw.WriteLine(string.Format(inv, "v {0} {1} {2}", x, wp.y, wp.z));
                }

                // 2) Faces (triangle winding)
                for (int sub = 0; sub < mesh.subMeshCount; sub++)
                {
                    int[] tris = mesh.GetTriangles(sub);
                    for (int i = 0; i < tris.Length; i += 3)
                    {
                        int a = tris[i] + 1 + vertexOffset;
                        int b = tris[i + 1] + 1 + vertexOffset;
                        int c = tris[i + 2] + 1 + vertexOffset;

                        // flipX로 handedness 뒤집었으면 와인딩도 뒤집어라 (b<->c)
                        if (flipX)
                        {
                            int tmp = b; b = c; c = tmp;
                        }

                        sw.WriteLine($"f {a} {b} {c}");
                    }
                }

                vertexOffset += mesh.vertexCount;
            }
        }

        Debug.Log("맵 추출 완료: " + path);
        EditorUtility.DisplayDialog("성공", "OBJ 추출 완료.", "OK");
    }
}
