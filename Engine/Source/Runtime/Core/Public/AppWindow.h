#pragma once

class FEngineLoop;

/**
 * @brief 입력 메시지 데이터 구조체
 */
struct FInputMessage
{
	uint32 Message;
	WPARAM WParam;
	LPARAM LParam;
};

/**
 * @brief 윈도우 생성 및 메시지 처리를 담당하는 클래스
 */
class FAppWindow
{
public:
	bool Init(HINSTANCE InInstance, int InCmdShow);
	static void InitializeConsole();

	// Viewport Information
	void GetClientSize(int32& OutWidth, int32& OutHeight) const;

	// Input Message Queue
	void ProcessPendingInputMessages();

	// Getter & Setter
	HWND GetWindowHandle() const { return MainWindowHandle; }
	HINSTANCE GetInstanceHandle() const { return InstanceHandle; }
	void SetNewTitle(const wstring& InNewTitle) const;

	// Special Member Function
	FAppWindow(FEngineLoop* InOwner);
	~FAppWindow();

private:
    FEngineLoop* Owner;
    HINSTANCE InstanceHandle;
    HWND MainWindowHandle;

	// Input Message Queue
	queue<FInputMessage> InputMessageQueue;
	mutex InputQueueMutex;

	void EnqueueInputMessage(uint32 InMessage, WPARAM InWParam, LPARAM InLParam);

    // Windows Callback Function
    static LRESULT CALLBACK WndProc(HWND InWindowHandle, uint32 InMessage, WPARAM InWParam, LPARAM InLParam);

    // Message Handle Function
    static FAppWindow* GetWindowInstance(HWND InWindowHandle, uint32 InMessage, LPARAM InLParam);
};
