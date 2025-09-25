# GTL Engine
### Game Tech Lab Week4 (Team 1)

 ![Engine_Editor](Document/engine_editor.png)

<br>

## 📖 프로젝트 개요

본 프로젝트는 Game Tech Lab 과정의 일환으로 개발된 **C++ 및 DirectX 11 기반의 미니 게임 엔진**입니다. Windows 애플리케이션으로서, 기본적인 엔진 아키텍처와 3D 렌더링 파이프라인 구축에 중점을 두고 학습 및 개발을 진행했습니다.

<br>

## ✨ 주요 기능

* 🎮 **렌더링 엔진**: DirectX 11 API를 기반으로 3D 그래픽 렌더링을 지원합니다.
* 🎨 **에디터 UI**: 디버깅 및 실시간 객체 제어를 위해 **ImGui** 라이브러리를 통합했습니다.
* 📦 **에셋 관리**: 셰이더(`.hlsl`), 텍스처 등 필수 에셋을 로드하고 관리하는 기본 시스템을 갖추고 있습니다.
* 👁️ **뷰 모드**: **Lit**, **Unlit**, **Wireframe** 뷰 모드 간의 전환을 지원하여 다양한 관점에서 씬을 분석할 수 있습니다.
* 🌳 **씬 관리자**: 씬(Scene)에 배치된 모든 액터(Actor)의 목록을 계층 구조로 시각화하고 관리합니다.

<br>

## 🆕 Week 04 업데이트 요약

- Viewport/레이아웃
  - 싱글 ↔ 4분할 레이아웃 전환 애니메이션 추가 (부드러운 EaseInOutCubic 이징)
  - 스플리터 드래그 비율 저장/복원 및 우클릭 드래그 중 충돌 방지(캡처 고정) 처리
  - 레이아웃 전환 시 직교 뷰 재바인딩 및 즉시 리프 Rect 동기화로 깜빡임 최소화
- Viewport 툴바/조작(UI)
  - ViewMode(Lit/Unlit/WireFrame), ViewType(Perspective/Ortho 6방향) 즉시 변경 가능
  - 카메라 스피드 드롭다운/슬라이더, 그리드 셀 사이즈 드롭다운/슬라이더 제공
  - 우측 패널 폭을 고려한 뷰포트 영역 계산 및 툴바 높이 반영 렌더링
- 카메라/직교 뷰 개선
  - 직교 카메라 6방향(Top/Bottom/Left/Right/Front/Back) 공유 중심점 기반 패닝/줌
  - 마우스 휠로 직교 FOV 변경, RMB 드래그 시 월드 좌표계로 부드러운 패닝
  - 퍼스펙티브/직교 각각의 카메라 종횡비 및 VP 행렬 실시간 업데이트
  - 0번 뷰포트 퍼스펙티브 카메라 저장/불러오기 기능 추가
- 렌더링/머티리얼/메시
  - StaticMesh 전용 셰이더 파이프라인 도입, 섹션 단위 드로우 + 슬롯별 머티리얼 적용
  - .mtl 파싱 및 머티리얼 슬롯 자동 구성, 기본 머티리얼/텍스처 자동 적용
  - 바이너리 캐시(*.mesh) 저장/로드 경로 제공, 캐시 유효성 검사 및 폴백 처리
  - UV 스크롤링 기능(머티리얼 플래그 기반) 추가
- 에디터 품질 향상
  - 기즈모 회전/스케일/호버 정확도 개선, 오쏘 뷰에서 포커스/컨트롤 고도화
  - Outliner 검색/이름 변경, 우측 디테일 패널 컨트롤 확장, Viewport 버그 다수 수정
- 텍스트/빌보드
  - 한글 폰트 렌더링 지원(FontRenderer + 한글 폰트 아틀라스), 빌보드가 카메라를 항상 바라보도록 개선
- 오버레이/진단
  - OverlayManagerSubsystem 추가: stat fps/memory/all/none 명령으로 오버레이 토글
  - FPS 이동평균, 프로세스 메모리/동적 할당 카운트 표시, Direct2D/DirectWrite 지연 초기화
- 안정화/기타
  - OBJ 임포터 안정화(중복 정점 인덱스, 에러 케이스 보완), 로그/워크플로우/빌드 스크립트 개선
  - 스플리터 드래그 중 기즈모 충돌 방지, 다양한 크래시/찌그러짐 이슈 수정

<br>

## 💻 코드 구조

### **렌더링 시스템**

* `BatchLines`
    * 에디터에 표시되는 다양한 선(그리드, 바운딩 박스 등)을 **하나의 버퍼에 모아 단일 드로우 콜(Draw Call)로 렌더링(배칭)**하는 최적화 클래스입니다.

* `BoundingBoxLines`
    * 객체를 감싸는 직육면체 형태의 경계 상자(AABB, Axis-Aligned Bounding Box)를 그리기 위한 정점 데이터를 생성합니다. 이 데이터는 `BatchLines`에 전달되어 렌더링됩니다.

* `URenderer`
    * DirectX 11 렌더링 파이프라인 전반을 담당하는 핵심 클래스입니다. 다양한 프리미티브 타입과 멀티뷰포트 렌더링, StaticMesh 전용 셰이더 처리를 포함합니다.

### **뷰포트 & 카메라 시스템**

* `UViewportManager`
    * 4분할/싱글 뷰포트 레이아웃 관리, 스플리터 드래그, 뷰포트 애니메이션 전환을 담당합니다. 6방향 직교 카메라와 퍼스펙티브 카메라 바인딩을 처리합니다.

* `FViewportClient` & `FViewport`
    * 각 뷰포트의 뷰 타입(Perspective/Ortho), 뷰 모드(Lit/Unlit/WireFrame), 카메라 바인딩을 관리합니다. 입력 라우팅과 렌더 영역 계산을 담당합니다.

* `UViewportControlWidget`
    * 각 뷰포트 상단 툴바 UI를 렌더링합니다. ViewMode/ViewType 콤보박스, 카메라 스피드/그리드 설정, 레이아웃 전환 버튼을 제공합니다.

### **에셋 & 머티리얼 시스템**

* `UAssetManager`
    * OBJ 파일 파싱, 바이너리 캐시 관리, StaticMesh/Material/Texture 로딩을 총괄합니다. .mtl 파일 파싱과 머티리얼 슬롯 자동 구성을 처리합니다.

* `UStaticMesh` & `UStaticMeshComponent`
    * OBJ 모델 데이터와 렌더 버퍼를 관리합니다. 섹션별 머티리얼 적용, 바이너리 직렬화, AABB 계산 기능을 제공합니다.

* `UMaterial` & `FObjMaterialInfo`
    * .mtl에서 파싱한 머티리얼 정보(Diffuse/Normal/Specular 텍스처, 색상, 반사도 등)를 캡슐화합니다.

### **UI & 에디터**

* `BillBoardComponent` & `FontRenderer`
    * 항상 카메라를 향하는 3D 빌보드와 한글 폰트 아틀라스 기반 텍스트 렌더링 시스템입니다.

* `UTargetActorTransformWidget`
    * 선택된 액터의 Transform 편집, StaticMesh 교체, 머티리얼 오버라이드 설정을 제공하는 디테일 패널입니다.

### **서브시스템 아키텍처**

* `FSubsystemCollection<T>` & `USubsystem`
    * 엔진/에디터/게임인스턴스 레벨의 서브시스템들을 생명주기와 함께 관리하는 컨테이너 시스템입니다.

* `UOverlayManagerSubsystem`
    * Direct2D/DirectWrite 기반 오버레이 렌더링을 담당합니다. FPS, 메모리 사용량 통계 표시와 stat 명령어 처리를 제공합니다.

<br>

## 🛠️ 시작하기

### 요구 사항

* **OS**: Windows 10 이상
* **IDE**: Visual Studio 2022 이상
    * 반드시 **"C++를 사용한 데스크톱 개발"** 워크로드가 설치되어 있어야 합니다.
* **SDK**: Windows SDK (최신 버전 권장)

### 빌드 순서

1.  `git`을 사용하여 이 저장소를 로컬 컴퓨터에 복제합니다.
    ```bash
    git clone [저장소 URL]
    ```
2.  Visual Studio에서 `GTL03.sln` 솔루션 파일을 엽니다.
3.  솔루션 구성을 `Debug` 또는 `Release` 모드로, 플랫폼을 `x64`로 설정합니다.
4.  메뉴에서 `빌드 > 솔루션 빌드`를 선택하거나 단축키 `F7`을 눌러 프로젝트를 빌드합니다.
5.  빌드가 성공하면 `Build/Debug` 또는 `Build/Release` 디렉터리에서 실행 파일(`GTL03.exe`)을 찾을 수 있습니다.

<br>

## 📂 프로젝트 구조
├───Engine/             메인 엔진 프로젝트 소스 코드<br>
│   ├───Core/           애플리케이션 프레임워크 (AppWindow, ClientApp)<br>
│   ├───Editor/         에디터 관련 기능 (카메라, 기즈모, 그리드)<br>
│   ├───Global/         전역 타입, 수학 유틸리티 (벡터, 행렬)<br>
│   ├───Mesh/           액터 및 컴포넌트, 기하학적 프리미티브<br>
│   ├───Render/         렌더링 시스템 (Device, Context, Shaders...)<br>
│   ├───Runtime/        엔진 런타임, Subsystem, Actor/Component, Level 등<br>
│   ├───Manager/        Viewport/Asset/UI/Level 등 매니저 모듈들<br>
│   └───Asset/          기본 에셋 (셰이더, 텍스처, 폰트)
│<br>
├───External/           외부 라이브러리 (DirectXTK, ImGui, json...)<br>
├───Document/           프로젝트 관련 문서<br>
├───GTL03.sln           Visual Studio 솔루션 파일<br>
└───README.md           이 파일<br>
