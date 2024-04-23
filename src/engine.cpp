#include "engine.h"

enum state {start, play, over};
state screen;

// Instructions variables to keep track of the elapsed time
float elapsedTime = 0.0f;
bool showText = true;

// Colors
color originalFill, hoverFill, pressFill;

Engine::Engine() : keys() {
    this->initWindow();
    this->initShaders();
    this->initShapes();

    originalFill = {1, 0, 0, 1};
    hoverFill.vec = originalFill.vec + vec4{0.5, 0.5, 0.5, 0};
    pressFill.vec = originalFill.vec - vec4{0.5, 0.5, 0.5, 0};
}

Engine::~Engine() {}

unsigned int Engine::initWindow(bool debug) {
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, false);

    window = glfwCreateWindow(width, height, "engine", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    // OpenGL configuration
    glViewport(0, 0, width, height);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glfwSwapInterval(1);

    return 0;
}

void Engine::initShaders() {
    // load shader manager
    shaderManager = make_unique<ShaderManager>();

    // Load shader into shader manager and retrieve it
    shapeShader = this->shaderManager->loadShader("../res/shaders/shape.vert", "../res/shaders/shape.frag",  nullptr, "shape");

    // Configure text shader and renderer
    textShader = shaderManager->loadShader("../res/shaders/text.vert", "../res/shaders/text.frag", nullptr, "text");
    fontRenderer = make_unique<FontRenderer>(shaderManager->getShader("text"), "../res/fonts/MxPlus_IBM_BIOS.ttf", 24);

    // Set uniforms
    textShader.setVector2f("vertex", vec4(100, 100, .5, .5));
    shapeShader.use();
    shapeShader.setMatrix4("projection", this->PROJECTION);
}

void Engine::initShapes() {
    // red spawn button centered in the top left corner
    spawnButton = make_unique<Rect>(shapeShader, vec2{width/2,height/2}, vec2{100, 50}, color{1, 0, 0, 1});
}

void Engine::processInput() {
    glfwPollEvents();

    // Set keys to true if pressed, false if released
    for (int key = 0; key < 1024; ++key) {
        if (glfwGetKey(window, key) == GLFW_PRESS)
            keys[key] = true;
        else if (glfwGetKey(window, key) == GLFW_RELEASE)
            keys[key] = false;
    }

    // Close window if escape key is pressed
    if (keys[GLFW_KEY_ESCAPE])
        glfwSetWindowShouldClose(window, true);

    // Go back to start screen if left arrow key is pressed
    if (keys[GLFW_KEY_LEFT])
        screen = start;

    // Mouse position saved to check for collisions
    glfwGetCursorPos(window, &MouseX, &MouseY);

    // TODO: If we're in the start screen and the user presses s, change screen to play
    if (screen == start && keys[GLFW_KEY_S])
        screen = play;

//    // TODO: If we're in the play screen and an arrow key is pressed, move the spawnButton
//    // Hint: one of the indices is GLFW_KEY_UP
//    if(screen == play && keys[GLFW_KEY_UP] && spawnButton->getPosY() <= 600){
//        spawnButton->moveY(1);
//    }
//    if(screen == play && keys[GLFW_KEY_DOWN] && spawnButton->getPosY() >= 0){
//        spawnButton->moveY(-1);
//    }
//    if(screen == play && keys[GLFW_KEY_LEFT] && spawnButton->getPosX() >= 0){
//        spawnButton->moveX(-1);
//    }
//    if(screen == play && keys[GLFW_KEY_RIGHT] && spawnButton->getPosX() <= 800){
//        spawnButton->moveX(1);
//    }
    // TODO: Make sure the spawnButton cannot go off the screen


    // Mouse position is inverted because the origin of the window is in the top left corner
    MouseY = height - MouseY; // Invert y-axis of mouse position
    bool buttonOverlapsMouse = spawnButton->isOverlapping(vec2(MouseX, MouseY));
    bool mousePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    // TODO: When in play screen, if the user hovers or clicks on the button then change the spawnButton's color
    // Hint: look at the color objects declared at the top of this file
    if(screen == play && (buttonOverlapsMouse || mousePressed)){
        spawnButton->setRed(100);
    }
    // TODO: When in play screen, if the button was released then spawn confetti
    // Hint: the button was released if it was pressed last frame and is not pressed now
    if(screen == play && mousePressed == false && mousePressedLastFrame == true){
        spawnConfetti();
    }
    // TODO: Make sure the spawn button is its original color when the user is not hovering or clicking on it.
    if(screen == play && !(buttonOverlapsMouse && mousePressed)){
        spawnButton->setRed(10);
    }

    // Save mousePressed for next frame
    mousePressedLastFrame = mousePressed;

}

void Engine::update() {
    // Calculate delta time
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // TODO: End the game when the user spawns 100 confetti
    // If the size of the confetti vector reaches 100, change screen to over
    if(confetti.size() >= 100){
        screen = over;
    }
}

void Engine::render() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Set background color
    glClear(GL_COLOR_BUFFER_BIT);

    // Set shader to use for all shapes
    shapeShader.use();

    // Render differently depending on screen
    switch (screen) {

        case start: {
            // Set background color
            glClearColor(0.913f, 0.662f, 0.784f, 1.0f); // Light pink
            // Clear the color buffer
            glClear(GL_COLOR_BUFFER_BIT);

            string title = "Piano Play";
            // Displayed at top of screen
            this->fontRenderer->renderText(title, width / 2 - (20 * title.length()), height / 1.25, 1.75, vec3{1, 1, 1});

            // Each sentence
            string sentence1 = "Welcome to our interactive piano practice program!";
            string sentence2 = "Play for fun or practice a short song";
            //string sentence3 = "Switch off all the lights with the least number of clicks.";

            // Positioning, size, color
            this->fontRenderer->renderText(sentence1, width / 6.5 - (5 * title.length()), height / 2 + 50, 0.58,vec3{0.808, 0.396, 0.667});
            this->fontRenderer->renderText(sentence2, width / 6.5 - (5 * title.length()), height / 2, 0.58, vec3{0.808, 0.396, 0.667});
            //this->fontRenderer->renderText(sentence3, width / 6.5 - (5 * title.length()), height / 2 - 50, 0.5,vec3{0.871, 0.055, 0.8});

            string fun = "Press S to play for fun";
            // (12 * message.length()) is the offset to center text.
            // 12 pixels is the width of each character scaled by 1.
            this->fontRenderer->renderText(fun, width / 2 - (12 * fun.length()), height / 5, 0.9, vec3{0.604, 0.325, 0.6});

        //TODO: configure P key to bring user to practice screen
            string practice = "Press P to practice a song";
            // (12 * message.length()) is the offset to center text.
            // 12 pixels is the width of each character scaled by 1.
            this->fontRenderer->renderText(practice, width / 2 - (12 * practice.length()), height / 5 + 50, 0.9, vec3{0.604, 0.325, 0.6});

            // Reset elapsedTime every time user is on start screen
            elapsedTime = 0.0f;
            break;

        }



        //TODO: Change case name to fun screen
        case play: {

            // Check if 5 seconds have passed to hide the text
            if (elapsedTime < 5.0f) {

                glClearColor(0.596f, 0.714f, 0.929f, 1.0f); // Light blue background
                // Clear the color buffer
                glClear(GL_COLOR_BUFFER_BIT);

                string title = "Instructions";
                // Displayed at top of screen
                this->fontRenderer->renderText(title, width / 1.8 - (20 * title.length()), height / 1.25, 1.5, vec3{0.996, 0.796, 0.243});

//                // Each instruction
//                string i1 = ">> Simply click on the piano keys to produce sound";
//                this->fontRenderer->renderText(i1, width / 2.7 - (5 * i1.length()), height - 230, 0.62, vec3{0.984, 0.945, 0.933});
//
//                string i2 = ">> Press esc to exit";
//                this->fontRenderer->renderText(i2, width / 4 - (6 * i2.length()), height - 270, 0.62, vec3{0.984, 0.945, 0.933});
//
//                string i3 = ">> Press the left arrow key at any point to return to the home screen";
//                this->fontRenderer->renderText(i3, width / 1 - (40 * i3.length()), height - 310, 0.40, vec3{0.984, 0.945, 0.933});


                int leftAlign = 50;
                int verticalSpacing = 40;

                string i1 = ">> Simply click on the piano keys to produce sound";
                this->fontRenderer->renderText(i1, leftAlign, height - 230, 0.60, vec3{0.984, 0.945, 0.933});

                string i2 = ">> Press esc to exit";
                this->fontRenderer->renderText(i2, leftAlign, height - 230 - verticalSpacing, 0.60, vec3{0.984, 0.945, 0.933});

                string i3 = ">> Press the left arrow key to return home";
                this->fontRenderer->renderText(i3, leftAlign, height - 230 - 2 * verticalSpacing, 0.60, vec3{0.984, 0.945, 0.933});

            } else {
                showText = false;
            }

            // Increment the elapsed time
            elapsedTime += deltaTime; // deltaTime is the time since the last frame

            for(const unique_ptr<Shape>& r: confetti){
                r->setUniforms();
                r->draw();
            }
            spawnButton->setUniforms();
            spawnButton->draw();

            break;
        }

        //TODO: Add a practice screen here

        case over: {
            // Set background color
            glClearColor(0.913f, 0.662f, 0.784f, 1.0f); // Light pink
            // Clear the color buffer
            glClear(GL_COLOR_BUFFER_BIT);

            string message = "You win!";
            // TODO: Display the message on the screen
            this->fontRenderer->renderText(message, width/2 - (12 * message.length()), height/2, 1, vec3{0.604, 0.325, 0.6});
            break;
        }
    }

    glfwSwapBuffers(window);
}

void Engine::spawnConfetti() {
    vec2 pos = {rand() % (int)width, rand() % (int)height};
    // TODO: Make each piece of confetti a different size, getting bigger with each spawn.
    //  The smallest should be a square of size 1 and the biggest should be a square of size 100
    int rd = 1 + (rand() % 100);
    vec2 size = {rd, rd}; // placeholder
    color color = {float(rand() % 10 / 10.0), float(rand() % 10 / 10.0), float(rand() % 10 / 10.0), 1.0f};
    confetti.push_back(make_unique<Rect>(shapeShader, pos, size, color));
}

bool Engine::shouldClose() {
    return glfwWindowShouldClose(window);
}

GLenum Engine::glCheckError_(const char *file, int line) {
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        string error;
        switch (errorCode) {
            case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
            case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
            case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        cout << error << " | " << file << " (" << line << ")" << endl;
    }
    return errorCode;
}