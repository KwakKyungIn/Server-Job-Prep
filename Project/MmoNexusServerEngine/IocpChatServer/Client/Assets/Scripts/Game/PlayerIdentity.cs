using UnityEngine;

/// <summary>
/// Lightweight identity component for world-interaction (right-click, etc.).
/// Attached at runtime in ObjectManager.Spawn.
/// </summary>
public class PlayerIdentity : MonoBehaviour
{
    public ulong PlayerId { get; private set; }
    public string PlayerName { get; private set; }
    public bool IsMine { get; private set; }

    public void Init(ulong playerId, string playerName, bool isMine)
    {
        PlayerId = playerId;
        PlayerName = playerName ?? string.Empty;
        IsMine = isMine;
    }
}
