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

#ifndef OPENGLWINDOW_HPP_INCLUDED
#define OPENGLWINDOW_HPP_INCLUDED

#include <QtGui/QWindow>
#include <QtGui/QOpenGLFunctions>


#include <Windows.h>
#include <windowsx.h>


class QPainter;
class QOpenGLContext;
class QOpenGLPaintDevice;

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

class OpenGLWindow : public QWindow, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit OpenGLWindow(QWindow *parent = 0);
    ~OpenGLWindow();

    virtual void render(QPainter *painter);
    virtual void render();

    virtual void initialize();

    void setAnimating(bool animating);

public slots:
    void renderLater();
    void renderNow();

protected:
    bool event(QEvent *event) override;

    void exposeEvent(QExposeEvent *event) override;

    bool nativeEvent(const QByteArray &eventType, void *message, long *result);

private:
    bool DWMEnabled(void);
    void adjust_maximized_client_rect(HWND window, RECT& rect);
    long ncHitTest(const MSG* wMsg) const;

    void setupUI();

    bool m_animating;

    tDwmDefWindowProc pDwmDefWindowProc;
    tDwmIsCompositionEnabled pDwmIsCompositionEnabled;
    tDwmExtendFrameIntoClientArea pDwmExtendFrameIntoClientArea;
    tGetWindowPlacement pGetWindowPlacement;
    tMonitorFromWindow pMonitorFromWindow;
    tGetMonitorInfoW pGetMonitorInfoW;
    tGetWindowRect pGetWindowRect;
    tAdjustWindowRectEx pAdjustWindowRectEx;
    tSetWindowLongPtrW pSetWindowLongPtrW;

    QOpenGLContext *m_context;
    QOpenGLPaintDevice *m_device;
};

#endif //OPENGLWINDOW_HPP_INCLUDED
