#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <QMainWindow>

#include <QWebEngineView>

#include <QLibrary>

#include <Windows.h>
#include <windowsx.h>

struct WMargins {
    int cxLeftWidth;
    int cxRightWidth;
    int cyTopHeight;
    int cyBottomHeight;
};

typedef BOOL (*tDwmDefWindowProc)(HWND, UINT, WPARAM, LPARAM, LRESULT*);
typedef HRESULT (*tDwmIsCompositionEnabled)(BOOL*);
typedef HRESULT (*tDwmExtendFrameIntoClientArea)(HWND hWnd, const WMargins*);
typedef BOOL (*tGetWindowPlacement)(HWND hWnd, WINDOWPLACEMENT *lpwndpl);
typedef HMONITOR (*tMonitorFromWindow)(HWND hWnd, DWORD dwFlags);
typedef BOOL (*tGetMonitorInfoW)(HMONITOR hMonitor, LPMONITORINFO lpmi);
typedef BOOL (*tGetWindowRect)(HWND hWnd, LPRECT lpRect);
typedef BOOL (*tAdjustWindowRectEx)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle);
typedef LONG_PTR (*tSetWindowLongPtrW)(HWND hWnd, int nIndex, LONG_PTR dwNewLong);

class Window : public QMainWindow
{
	Q_OBJECT
public:
	explicit Window(QWidget *parent = nullptr);
	~Window() = default;

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, long *result);
    long ncHitTest(const MSG* wMsg) const;

private:
	void setupUI();

    bool DWMEnabled(void);
    void adjust_maximized_client_rect(HWND window, RECT& rect);

    tDwmDefWindowProc pDwmDefWindowProc;
    tDwmIsCompositionEnabled pDwmIsCompositionEnabled;
    tDwmExtendFrameIntoClientArea pDwmExtendFrameIntoClientArea;
    tGetWindowPlacement pGetWindowPlacement;
    tMonitorFromWindow pMonitorFromWindow;
    tGetMonitorInfoW pGetMonitorInfoW;
    tGetWindowRect pGetWindowRect;
    tAdjustWindowRectEx pAdjustWindowRectEx;
    tSetWindowLongPtrW pSetWindowLongPtrW;

	QWebEngineView* m_view{nullptr};
};

#endif // WINDOW_HPP
