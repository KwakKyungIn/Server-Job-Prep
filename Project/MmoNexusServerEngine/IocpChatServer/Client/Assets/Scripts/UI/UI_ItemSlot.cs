using UnityEngine;
using UnityEngine.UI;
using Protocol; // ItemInfo 쓰려면 필요

public class UI_ItemSlot : MonoBehaviour
{
    [Header("UI Components")]
    public Image _icon;
    public Text _countText; // 혹은 TMPro.TMP_Text
    public Image _equipFrame; // 장착 중일 때 띄울 테두리 (없으면 패스)

    private ItemInfo _item;

    public void SetItem(ItemInfo item)
    {
        _item = item;

        if (item == null)
        {
            // 빈 슬롯 처리
            _icon.gameObject.SetActive(false);
            _countText.text = "";
            if (_equipFrame) _equipFrame.gameObject.SetActive(false);
            return;
        }

        // 아이템 정보 표시
        _icon.gameObject.SetActive(true);
        _countText.text = (item.Count > 1) ? item.Count.ToString() : ""; // 1개면 숫자 안 띄움

        // 장착 표시 (테두리 켜기/끄기)
        if (_equipFrame)
            _equipFrame.gameObject.SetActive(item.IsEquipped);

        // [TODO] 아이콘 로드 (지금은 임시로 색깔만 바꿈 or 템플릿ID로 스프라이트 로드)
        // LoadIcon(item.TemplateId);

        // 디버깅용: 101번(검)은 빨강, 102번(갑옷)은 파랑 (리소스 없을 때 임시)
        if (item.TemplateId == 101) _icon.color = Color.red;
        else if (item.TemplateId == 102) _icon.color = Color.blue;
        else _icon.color = Color.white;
    }

    // 클릭했을 때 장착/해제 요청 보내기 (나중에 Button 컴포넌트 연결)
    public void OnClickSlot()
    {
        if (_item == null) return;

        Debug.Log($"[UI] Click Slot: {_item.TemplateId}");

        C_EQUIP_ITEM pkt = new C_EQUIP_ITEM();
        pkt.ItemUid = _item.ItemUid;
        pkt.SlotIndex = _item.Slot;
        pkt.Equip = !_item.IsEquipped; // 토글

        NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_EQUIP_ITEM);
    }
}