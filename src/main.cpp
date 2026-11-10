#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "app.h"
#include "config.h"

using namespace std;

int main(int argc, char** args) {
  srand(static_cast<int>(time(nullptr)));

  config.win_w = 1200;
  config.win_h = 800;

  App app;
  app.init();

  if (argc >= 2) app.loadSourceFile(args[1]);

  app.run();

  return EXIT_SUCCESS;
}
