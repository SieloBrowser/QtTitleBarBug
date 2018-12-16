#include "Window.hpp"

bool Window::DWMEnabled(void)
{
    if (QSysInfo::windowsVersion() < QSysInfo::WV_VISTA) return false;
    BOOL useDWM;

    if (pDwmIsCompositionEnabled(&useDWM) < 0) return false;
    return useDWM == TRUE;
}

void Window::adjust_maximized_client_rect(HWND window, RECT& rect)
{
    WINDOWPLACEMENT placement;
    if (!pGetWindowPlacement(window, &placement) || placement.showCmd != SW_MAXIMIZE)
        return;

    auto monitor = pMonitorFromWindow(window, MONITOR_DEFAULTTONULL);
    if (!monitor)
        return;

    MONITORINFO monitor_info{};
    monitor_info.cbSize = sizeof(monitor_info);
    if (!pGetMonitorInfoW(monitor, &monitor_info))
        return;

    // when maximized, make the client area fill just the monitor (without task bar) rect,
    // not the whole window rect which extends beyond the monitor.
    rect = monitor_info.rcWork;
}

Window::Window(QWidget *parent) :
	QMainWindow(parent)
{
	setObjectName("mainwindow");

	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle(tr("Bug Sample"));

	setupUI();
}

void Window::setupUI()
{
    {
        QLibrary user32("User32");
        if (user32.load()) {
            pGetWindowPlacement = reinterpret_cast<tGetWindowPlacement>(user32.resolve("GetWindowPlacement"));
            pMonitorFromWindow = reinterpret_cast<tMonitorFromWindow>(user32.resolve("MonitorFromWindow"));
            pGetMonitorInfoW = reinterpret_cast<tGetMonitorInfoW>(user32.resolve("GetMonitorInfoW"));
            pGetWindowRect = reinterpret_cast<tGetWindowRect>(user32.resolve("GetWindowRect"));
            pAdjustWindowRectEx = reinterpret_cast<tAdjustWindowRectEx>(user32.resolve("AdjustWindowRectEx"));
            pSetWindowLongPtrW = reinterpret_cast<tSetWindowLongPtrW>(user32.resolve("SetWindowLongPtrW"));
        }
    }
    {
        QLibrary dwmApi("dwmapi");
        if (dwmApi.load()) {
            pDwmDefWindowProc = reinterpret_cast<tDwmDefWindowProc>(dwmApi.resolve("DwmDefWindowProc"));
            pDwmExtendFrameIntoClientArea = reinterpret_cast<tDwmExtendFrameIntoClientArea>(
                dwmApi.resolve("DwmExtendFrameIntoClientArea"));
            pDwmIsCompositionEnabled = reinterpret_cast<tDwmIsCompositionEnabled>(
                dwmApi.resolve("DwmIsCompositionEnabled"));
        }
    }

    setWindowFlags(Qt::FramelessWindowHint);
    pSetWindowLongPtrW(reinterpret_cast<HWND>(winId()), GWL_STYLE, WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX);
    const WMargins margins = {1,1,1,1};
    pDwmExtendFrameIntoClientArea(reinterpret_cast<HWND>(winId()), &margins);

	resize(1024, 723);

	m_view = new QWebEngineView(this);
	m_view->load(QUrl("https://sielo.app/"));

	setCentralWidget(m_view);
}

bool Window::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType);
    const MSG* wMsg = reinterpret_cast<MSG*>(message);
    const UINT wMessage = wMsg->message;
    bool hasHandled = false;
    long res = 0;

    if (DWMEnabled())
        hasHandled = pDwmDefWindowProc(wMsg->hwnd, wMessage, wMsg->wParam,
                                      wMsg->lParam, reinterpret_cast<LRESULT*>(&res));

    if (wMessage == WM_NCCALCSIZE && wMsg->wParam == TRUE) {
        NCCALCSIZE_PARAMS& params = *reinterpret_cast<NCCALCSIZE_PARAMS*>(wMsg->lParam);
        adjust_maximized_client_rect(wMsg->hwnd, params.rgrc[0]);
        res = 0;
        hasHandled = true;
    }
    else if (wMessage == WM_NCHITTEST && res == 0) {
        res = ncHitTest(wMsg);

        if (res != HTNOWHERE)
            hasHandled = true;
    }
    //else if (wMessage == WM_NCPAINT)
    //	hasHandled = true;

    if (hasHandled)
        *result = res;
    return hasHandled;
}

long Window::ncHitTest(const MSG* wMsg) const
{
    const long ncHitZone[3][4] = {
        {HTTOPLEFT, HTLEFT, HTLEFT, HTBOTTOMLEFT},
        {HTTOP, HTCAPTION, HTNOWHERE, HTBOTTOM},
        {HTTOPRIGHT, HTRIGHT, HTRIGHT, HTBOTTOMRIGHT}
    };

    const QPoint cursor(GET_X_LPARAM(wMsg->lParam), GET_Y_LPARAM(wMsg->lParam));
    const UINT8 borderSize = 2;
    UINT8 xPos = 1;
    UINT8 yPos = 2;

    RECT rcWin;
    pGetWindowRect(wMsg->hwnd, &rcWin);

    /*if (m_captionWidget == QApplication::widgetAt(QCursor::pos()))
        return HTCAPTION;*/

    RECT rcFrame = {0};
    pAdjustWindowRectEx(&rcFrame, WS_OVERLAPPEDWINDOW & ~WS_CAPTION, FALSE, NULL);

    if (cursor.y() >= rcWin.top && cursor.y() <= rcWin.top + borderSize)
        yPos = 0;
    else if (cursor.y() >= rcWin.top + borderSize && cursor.y() <= rcWin.top + borderSize)
        yPos = 1;
    else if (cursor.y() >= rcWin.bottom - borderSize && cursor.y() < rcWin.bottom)
        yPos = 3;

    if (cursor.x() >= rcWin.left && cursor.x() < rcWin.left + borderSize)
        xPos = 0;
    else if (cursor.x() >= rcWin.right - borderSize && cursor.x() < rcWin.right)
        xPos = 2;

    return ncHitZone[xPos][yPos];
}
