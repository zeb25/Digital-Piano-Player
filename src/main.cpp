
#include "engine.h"
#include <iostream>

 int main(int argc, char *argv[]) {
    Engine engine;

    while (!engine.shouldClose()) {
        engine.processInput();
        engine.render();
        engine.update();
    }

    glfwTerminate();
    return 0;
}

