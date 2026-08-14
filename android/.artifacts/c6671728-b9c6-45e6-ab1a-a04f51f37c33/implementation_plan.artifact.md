# 프로젝트 구조 리팩토링 및 클린 아키텍처 적용

현재 `MainActivity.kt`와 `MainViewModel.kt`에 집중되어 있는 코드를 관심사 분리 원칙에 따라 체계적으로 분리하여 포트폴리오 수준의 전문적인 코드 구조로 개편합니다.

## 제안하는 변경 사항

### 1. 패키지 구조 재구성
- `network`: 소켓 통신 및 데이터 송수신 엔진 분리
- `ui`: 화면 구성 요소, 상태 관리, 테마 관련 파일
- `util`: 이미지 처리(YUV to JPEG) 및 회전 로직 독립화

### 2. 컴포넌트별 리팩토링 계획

#### [NEW] [TcpSocketClient.kt](file:///C:/Users/kccistc/AndroidStudioProjects/TcpMediaBridge/app/src/main/java/com/hocheol/tcpmediabridge/network/TcpSocketClient.kt)
- ViewModel에서 소켓 로직을 분리하여 재사용 가능하고 테스트 가능한 독립 클래스로 구현
- 연결 상태 알림 및 메시지 수신을 위한 콜백/Flow 인터페이스 제공

#### [NEW] [ImageUtils.kt](file:///C:/Users/kccistc/AndroidStudioProjects/TcpMediaBridge/app/src/main/java/com/hocheol/tcpmediabridge/util/ImageUtils.kt)
- `ImageProxy` 변환 및 비트맵 회전 로직을 별도 유틸리티 파일로 이동
- 확장 함수(Extension Functions)를 사용하여 가독성 향상

#### [MODIFY] [MainViewModel.kt](file:///C:/Users/kccistc/AndroidStudioProjects/TcpMediaBridge/app/src/main/java/com/hocheol/tcpmediabridge/ui/MainViewModel.kt)
- `TcpSocketClient`를 사용하여 비즈니스 로직 단순화
- UI 상태(State) 관리에 집중

#### [MODIFY] [MainActivity.kt](file:///C:/Users/kccistc/AndroidStudioProjects/TcpMediaBridge/app/src/main/java/com/hocheol/tcpmediabridge/MainActivity.kt)
- 진입점 역할 및 권한 관리 집중
- 복잡한 UI 컴포넌트들을 별도 파일로 분리 고려 (또는 하단부 정리)

## 검증 계획
### 자동화 테스트
- 리팩토링 후 빌드 성공 여부 확인
- 런타임 시 소켓 연결 및 영상 스트리밍 정상 동작 확인

### 수동 검증
- 카메라 미리보기 및 회전 보정 로직 정상 작동 여부 재확인
- UI 컴포넌트 간의 상태 연동 확인
