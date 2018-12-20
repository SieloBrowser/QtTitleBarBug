/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "openglwindow.h"

#include <QtCore/QCoreApplication>

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLPaintDevice>
#include <QtGui/QPainter>

#include <QtCore/QLibrary>

bool OpenGLWindow::DWMEnabled(void)
{
    if (QSysInfo::windowsVersion() < QSysInfo::WV_VISTA) return false;
    BOOL useDWM;

    if (pDwmIsCompositionEnabled(&useDWM) < 0) return false;
    return useDWM == TRUE;
}

void OpenGLWindow::adjust_maximized_client_rect(HWND window, RECT& rect)
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

OpenGLWindow::OpenGLWindow(QWindow *parent)
    : QWindow(parent)
    , m_animating(false)
    , m_context(0)
    , m_device(0)
{
    setupUI();
    setSurfaceType(QWindow::OpenGLSurface);
}

void OpenGLWindow::setupUI()
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

    //setWindowFlags(Qt::FramelessWindowHint);
    setFlag(Qt::FramelessWindowHint);
    pSetWindowLongPtrW(reinterpret_cast<HWND>(winId()), GWL_STYLE, WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX);
    const WMargins margins = {1,1,1,1};
    pDwmExtendFrameIntoClientArea(reinterpret_cast<HWND>(winId()), &margins);
}

OpenGLWindow::~OpenGLWindow()
{
    delete m_device;
}
void OpenGLWindow::render(QPainter *painter)
{
    Q_UNUSED(painter);
}

void OpenGLWindow::initialize()
{
}

void OpenGLWindow::render()
{
    if (!m_device)
        m_device = new QOpenGLPaintDevice;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    m_device->setSize(size());

    QPainter painter(m_device);
    render(&painter);
}

void OpenGLWindow::renderLater()
{
    requestUpdate();
}

bool OpenGLWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::UpdateRequest:
        renderNow();
        return true;
    default:
        return QWindow::event(event);
    }
}

void OpenGLWindow::exposeEvent(QExposeEvent *event)
{
    Q_UNUSED(event);

    if (isExposed())
        renderNow();
}

void OpenGLWindow::renderNow()
{
    if (!isExposed())
        return;

    bool needsInitialize = false;

    if (!m_context) {
        m_context = new QOpenGLContext(this);
        m_context->setFormat(requestedFormat());
        m_context->create();

        needsInitialize = true;
    }

    m_context->makeCurrent(this);

    if (needsInitialize) {
        initializeOpenGLFunctions();
        initialize();
    }

    render();

    m_context->swapBuffers(this);

    if (m_animating)
        renderLater();
}

void OpenGLWindow::setAnimating(bool animating)
{
    m_animating = animating;

    if (animating)
        renderLater();
}

bool OpenGLWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
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

long OpenGLWindow::ncHitTest(const MSG* wMsg) const
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
