#include "editor_window.hpp"
#include <gtkmm/application.h>

int main(int argc, char *argv[])
{
  auto app = Gtk::Application::create(argc, argv, "de.primeapi.primecuts.editor");

  EditorWindow window;

  return app->run(window);
}
