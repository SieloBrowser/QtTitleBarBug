#include <QApplication>

#include "Window.hpp"

int main(int argc, char** argv) {
	QApplication app{argc, argv};

	Window* window{new Window()};
	window->show();

	return app.exec();
}
