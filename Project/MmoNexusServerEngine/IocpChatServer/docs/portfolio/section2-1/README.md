# Section 2-1 Material Pack (Page 6~8)

Section 2-1 "서버 권위 이동 검증" 제작용 정리본입니다.

## 파일 구성
- `page6_server_authority_why.md`
  - Page 6 문안/문서 배치/제출 포인트
- `page7_movement_validation_pipeline.md`
  - Page 7 문안/문서 배치/제출 포인트
- `page8_placeholder.md`
  - Page 8 사용자 직접 제작용 체크리스트
- `page6_server_authority_why.mmd`
  - Page 6용 위협-대응 Mermaid 소스
- `page7_movement_validation_pipeline.mmd`
  - Page 7용 검증 파이프라인 Mermaid 소스

## Outline 연계
- `docs/portfolio/Portfolio_Outline.md`
  - Page 6, Page 7: 구체화 완료
  - Page 8: 사용자 직접 제작 placeholder

## SVG 렌더 명령 (로컬 mermaid-cli 사용 시)
```bash
cd docs/portfolio/section2-1
mmdc -i page6_server_authority_why.mmd -o page6_server_authority_why.svg
mmdc -i page7_movement_validation_pipeline.mmd -o page7_movement_validation_pipeline.svg
```

`mmdc`가 없으면:
```bash
npx -y @mermaid-js/mermaid-cli -i page6_server_authority_why.mmd -o page6_server_authority_why.svg
npx -y @mermaid-js/mermaid-cli -i page7_movement_validation_pipeline.mmd -o page7_movement_validation_pipeline.svg
```
