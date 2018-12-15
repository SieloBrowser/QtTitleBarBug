#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <QMainWindow>

#include <QWebEngineView>

class Window : public QMainWindow
{
	Q_OBJECT
public:
	explicit Window(QWidget *parent = nullptr);
	~Window() = default;

private:
	void setupUI();

	QWebEngineView* m_view{nullptr};
};

#endif // WINDOW_HPP
