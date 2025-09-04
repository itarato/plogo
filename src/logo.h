#pragma once

#include <condition_variable>
#include <exception>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "vm.h"

void runLogoThread(std::condition_variable* queueCondVar, VM* vm, std::mutex* queueMutex,
                   std::queue<std::string>* queue) {
  while (true) {
    if (WindowShouldClose()) return;

    std::string code{};
    {
      TraceLog(LOG_INFO, "Thread wait");
      std::unique_lock guard(*queueMutex);
      queueCondVar->wait(guard, [&] { return !queue->empty() || WindowShouldClose(); });
      TraceLog(LOG_INFO, "Thread wake. Queue size = %d", queue->size());

      if (queue->empty()) {
        guard.unlock();
        continue;
      }

      code = queue->front();
      queue->pop();
      guard.unlock();
    }

    Lexer lexer{code};
    Parser parser{lexer.parse()};
    Ast::Program prg = parser.parse();

    try {
      vm->mutex.lock();
      prg.execute(vm);
      vm->mutex.unlock();
    } catch (runtime_error& e) {
      TraceLog(LOG_ERROR, "Compiler execution error: %s", e.what());
    }
  }
}
