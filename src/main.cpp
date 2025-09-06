#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "app.h"
#include "config.h"

using namespace std;

int main(int argc, char** args) {
  setlocale(LC_ALL, "");
  srand(time(nullptr));

  config.win_w = 1024;
  config.win_h = 768;

  App app;
  app.init();

  if (argc >= 2) app.setSourceFile(args[1]);

  app.run();

  return EXIT_SUCCESS;
}
