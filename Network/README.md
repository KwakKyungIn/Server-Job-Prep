# 🌐 네트워크 모델 정리

> 소켓 통신에서 사용되는 주요 I/O 모델을 비교·정리한 문서입니다.  
> TCP/UDP의 차이부터 Blocking, Non-Blocking, Select, WSAEventSelect, Overlapped, IOCP까지  
> 서버 프로그래밍에 필요한 네트워크 동작 방식을 단계적으로 다룹니다.  

---

## 📂 목차

| 번호 | 제목                                    | 설명                                   |
|------|---------------------------------------|--------------------------------------|
| 01   | [TCP vs UDP](01_Network_TCPvsUDP.md)   | 전송 계층의 두 가지 대표 프로토콜 비교 |
| 02   | [Blocking vs Non-Blocking](02_Network_BlockingVsNonBlocking.md) | 입출력 처리 방식의 차이 정리 |
| 03   | [Select Model](03_Network_SelectModel.md) | 멀티플렉싱 기반 I/O 모델 설명 |
| 04   | [WSAEventSelect](04_Network_WSAEventSelect.md) | 윈도우 이벤트 기반 모델 정리 |
| 05   | [Overlapped I/O](05_Network_Overlapped.md) | 비동기 입출력 방식 정리 |
| 06   | [IOCP 기반 게임 서버](06_Network_IOCP_GameServer.md) | 대규모 서버에 적합한 Completion Port 모델 설명 |

---

## ✍️ 설명 및 사용법

- 각 문서는 **개념 → 동작 방식 → 장단점 → 활용 사례** 순서로 정리되어 있습니다.  
- 실제 고성능 서버 개발에 필요한 네트워크 모델 이해를 목표로 합니다.  
