#include "Window.hpp"

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
	resize(1024, 723);

	m_view = new QWebEngineView(this);
	m_view->load(QUrl("https://sielo.app/"));

	setCentralWidget(m_view);
}
