#include <QApplication>

#include "Window.hpp"

int main(int argc, char** argv) {
	qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");

	QApplication app{argc, argv};

	Window* window{new Window()};
	window->show();

	return app.exec();
}
