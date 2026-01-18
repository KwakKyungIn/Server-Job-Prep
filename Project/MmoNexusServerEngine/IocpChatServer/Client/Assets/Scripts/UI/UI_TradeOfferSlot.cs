using TMPro;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;
using Protocol;

public class UI_TradeOfferSlot : MonoBehaviour, IDropHandler, IPointerClickHandler
{
    [Header("Wiring")]
    public bool allowDrop = true; // my side slot only
    public TMP_Text labelText;
    public Image iconImage;          // 아이템 아이콘
    public Sprite emptyIcon;         // 비었을 때 표시할 아이콘(선택)

    [Header("State (read-only)")]
    public ulong itemUid;
    public int templateId;
    public int count;

    public void SetData(ulong uid, int tId, int c)
    {
        itemUid = uid;
        templateId = tId;
        count = c;

        // 아이콘 반영
        if (iconImage != null)
        {
            if (uid == 0)
            {
                iconImage.sprite = emptyIcon;
                iconImage.enabled = (emptyIcon != null);
            }
            else
            {
                var sp = ItemIconDB.Get(tId);
                iconImage.sprite = sp;
                iconImage.enabled = (sp != null);
            }
        }

        if (labelText != null)
        {
            if (uid == 0)
                labelText.text = "(empty)";
            else
                labelText.text = $"{tId} x{c}\nuid:{uid}";
        }
    }

    public void Clear()
    {
        SetData(0, 0, 0);
    }

    public void OnDrop(PointerEventData eventData)
    {
        if (!allowDrop) return;
        if (!TradeManager.Instance.InTrade || TradeManager.Instance.Locked) return;

        if (!UI_DragPayload.HasPayload) return;

        var payload = UI_DragPayload.Current;
        if (payload.Type != UI_DragPayload.PayloadType.Item) return;
        if (payload.RefId == 0) return;

        // 부분 수량 지원:
        // - count==1이면 바로 제안
        // - count>1이면 입력 팝업으로 수량 선택
        var it = InventoryManager.Instance.GetItemByUid(payload.RefId);
        if (it == null) return;

        int maxCount = it.Count;
        if (maxCount <= 1)
        {
            TradeManager.Instance.TryOfferItem(payload.RefId, 1);
            return;
        }

        UI_TradeCountPrompt.Show(maxCount,
            onConfirm: (c) =>
            {
                if (!TradeManager.Instance.InTrade || TradeManager.Instance.Locked) return;
                TradeManager.Instance.TryOfferItem(payload.RefId, c);
            });
    }


    public void OnPointerClick(PointerEventData eventData)
    {
        if (eventData == null) return;
        if (!allowDrop) return;

        // 우클릭: 이 슬롯에 들어있는 아이템 제안 제거
        if (eventData.button == PointerEventData.InputButton.Right)
        {
            if (itemUid != 0)
                TradeManager.Instance.RemoveOffer(itemUid);
        }
    }
}
