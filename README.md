# GTL-Week03 Engine
### Game Tech Lab 3주차 - 7팀 프로젝트

 ![Engine_Editor](Document/engine_editor.jpg)

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

## 💻 코드 구조

* `BatchLines`
    * 에디터에 표시되는 다양한 선(그리드, 바운딩 박스 등)을 **하나의 버퍼에 모아 단일 드로우 콜(Draw Call)로 렌더링(배칭)**하는 최적화 클래스입니다.

* `BoundingBoxLines`
    * 객체를 감싸는 직육면체 형태의 경계 상자(AABB, Axis-Aligned Bounding Box)를 그리기 위한 정점 데이터를 생성합니다. 이 데이터는 `BatchLines`에 전달되어 렌더링됩니다.

* `BillBoardComponent`
    * 항상 카메라를 정면으로 바라보도록 자동 회전하는 3D 컴포넌트입니다. 3D 월드 상에 액터의 이름이나 아이콘 같은 2D 정보를 표시하는 데 유용하게 사용됩니다.

* `FontRenderer`
    * 폰트 아틀라스(Font Atlas) 텍스처를 사용하여 화면에 텍스트를 렌더링하는 클래스입니다. 셰이더 설정, 정점 버퍼 생성, 알파 블렌딩 등 텍스트 렌더링에 필요한 모든 과정을 캡슐화합니다.

* `SceneManagerWindow`
    * 씬에 존재하는 모든 액터를 계층 구조로 보여주는 ImGui 기반의 UI 창 클래스입니다. 내부적으로 `SceneHierarchyWidget`을 사용하여 실제 목록을 표시하고 상호작용을 관리합니다.

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
│   └───Asset/          기본 에셋 (셰이더, 텍스처, 폰트)<br>
│<br>
├───External/           외부 라이브러리 (DirectXTK, ImGui, json...)<br>
├───Document/           프로젝트 관련 문서<br>
├───GTL03.sln           Visual Studio 솔루션 파일<br>
└───REA DME.md           이 파일<br>
