#include "engine.h"
#include <vector>       // Include the vector header
#include <GLFW/glfw3.h> // Include GLFW header for key codes

enum state {start, freePlay, gamePlay, over};
state screen;

// Instructions variables to keep track of the elapsed time
float elapsedTime = 0.0f;
bool showText = true;

// Colors
color originalFill, hoverFill, pressFill, blackKey, whiteKey;

Engine::Engine() : keys() {
    this->initWindow();
    this->initShaders();
    this->initShapes();
    this->processInput();

    originalFill = {1, 0, 0, 1};
    pressFill = {0.204, 0.439, 0.78};
    hoverFill.vec = originalFill.vec + vec4{0.5, 0.5, 0.5, 0};

    blackKey = {0, 0, 0, 1};
    whiteKey = {1, 1, 1, 1};

    isPlaying = false;

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

    // Audio stream errors
    if( paInit.result() != paNoError ) {
        fprintf( stderr, "An error occurred while using the portaudio stream\n" );
        fprintf( stderr, "Error number: %d\n", paInit.result() );
        fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( paInit.result() ) );
        return -1;
    }

    // Opens sound streams
    sine.open(Pa_GetDefaultOutputDevice());
    sound_engine.run();

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
    // Width of each piano key
    double keyWidth = 90; // Dividing the screen into 7 keys

    // Add white keys (naturals)
    for(int i = 100; i <= 700; i += 100) {
        float keyX = i;
        float keyY = height / 4;
        piano.push_back(make_unique<Rect>(shapeShader, vec2{keyX, keyY}, vec2{keyWidth, height / 2}, color{1, 1, 1, 1}));
        // Get the index of the current key in the loop
        int keyIndex = (i - 100) / 100;
//        sounds.push_back(SoundEngine());
//        sounds[sounds.size()-1].run();
    }

    // Add black keys (sharps/flats)
    float blackKeyWidth = 85; // Arbitrary fraction of white key width for black keys
    float blackKeyHeight = height / 3; // Arbitrary height for black keys
    for(int i = 150; i <= 650; i += 100) {
        // Skip third index
        if (i == 350) {
            continue;
        }
        float keyX = i;
        float keyY = height / 4 * 1.5; // Offset from white keys
        piano.push_back(make_unique<Rect>(shapeShader, vec2{keyX, keyY}, vec2{blackKeyWidth, blackKeyHeight}, color{0, 0, 0, 1}));
        int keyIndex = (i - 150) / 100;
//        sounds.push_back(SoundEngine());
//        sounds[sounds.size()-1].run();
    }

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
    if (keys[GLFW_KEY_ESCAPE]) {
        glfwSetWindowShouldClose(window, true);
        sine.stop();
        sine.close();
        //sound_engine.stopSine();
    }

    // Go back to start screen if left arrow key is pressed
    if (keys[GLFW_KEY_LEFT]) {
        screen = start;
        //sound_engine.stopSine();
    }

    // Mouse position saved to check for collisions
    glfwGetCursorPos(window, &MouseX, &MouseY);

    // If we're in the start screen and the user presses s, change screen to free play screen
    if (screen == start && keys[GLFW_KEY_S]) {
        screen = freePlay;
        sine.stop();
        // sound_engine.makeSine(400);
    }

    // If we're in the start screen and the user presses p, change screen to play the games activity
    if (screen == start && keys[GLFW_KEY_P]) {
        screen = gamePlay;
        sine.stop();
        // sound_engine.makeSine();
    }

    // Mouse position is inverted because the origin of the window is in the top left corner
    MouseY = height - MouseY; // Invert y-axis of mouse position

    // figure out how to check if mouse is overlapping any piano key
    bool keyOverlapsMouse = piano[0]->isOverlapping(
            vec2(MouseX, MouseY)); // checks if mouse overlaps with first piano key

    bool mousePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

////// EACH PIANO KEY IS REPRESENTED BY A KEY ON THE KEYBOARD //////

//// WHITE KEYS ////

    if (screen == freePlay) {
        if (keys['Z'] && !keysLastFrame['Z']) {
            sine.start();
//            sounds[0].makeSine(460.0f);
            // Highlight C key when pressed
            if (!piano.empty()) {
                piano[0]->setColor(pressFill);
            }
        } else if(!keys['Z'] && keysLastFrame['Z']) {
            sine.stop();
            //sound_engine.stopSine(340.0);
            //sounds[0].stopSine(460.0f);
            // Reset color
            if (!piano.empty()) {
                piano[0]->setColor(whiteKey);
            }
        }
    }

    if (screen == freePlay) {
        if (keys['X'] && !keysLastFrame['X']) {
            sine.start();
//            sounds[0].makeSine(460.0f);
            // Highlight C key when pressed
            if (!piano.empty()) {
                piano[1]->setColor(pressFill);
            }
        } else if(!keys['X'] && keysLastFrame['X']) {
            sine.stop();
            //sound_engine.stopSine(340.0);
            //sounds[0].stopSine(460.0f);
            // Reset color
            if (!piano.empty()) {
                piano[1]->setColor(whiteKey);
            }
        }
    }

    if (screen == freePlay) {
        if (keys['C'] && !keysLastFrame['C']) {
            sine.start();
            // Highlight C key when pressed
            if (!piano.empty()) {
                piano[2]->setColor(pressFill);
            }
        } else if(!keys['C'] && keysLastFrame['C']) {
            sine.stop();
            //sound_engine.stopSine(340.0);
            //sounds[0].stopSine(460.0f);
            // Reset color
            if (!piano.empty()) {
                piano[2]->setColor(whiteKey);
            }
        }
    }

    if (screen == freePlay) {
        if (keys['V'] && !keysLastFrame['V']) {
            sine.start();
            if (!piano.empty()) {
                piano[3]->setColor(pressFill);
            }
        } else if(!keys['V'] && keysLastFrame['V']) {
            sine.stop();
            // Reset color
            if (!piano.empty()) {
                piano[3]->setColor(whiteKey);
            }
        }
    }

    if (screen == freePlay) {
        if (keys['B'] && !keysLastFrame['B']) {
            sine.start();
            if (!piano.empty()) {
                piano[4]->setColor(pressFill);
            }
        } else if(!keys['B'] && keysLastFrame['B']) {
            sine.stop();
            // Reset color
            if (!piano.empty()) {
                piano[4]->setColor(whiteKey);
            }
        }
    }

    if (screen == freePlay) {
        if (keys['N'] && !keysLastFrame['N']) {
            sine.start();
            if (!piano.empty()) {
                piano[5]->setColor(pressFill);
            }
        } else if(!keys['N'] && keysLastFrame['N']) {
            sine.stop();
            // Reset color
            if (!piano.empty()) {
                piano[5]->setColor(whiteKey);
            }
        }
    }

    if (screen == freePlay) {
        if (keys['M'] && !keysLastFrame['M']) {
            sine.start();
            if (!piano.empty()) {
                piano[6]->setColor(pressFill);
            }
        } else if(!keys['M'] && keysLastFrame['M']) {
            sine.stop();
            // Reset color
            if (!piano.empty()) {
                piano[6]->setColor(whiteKey);
            }
        }
    }

    //// Black KEYS ////

    if (screen == freePlay) {
        if (keys['S'] && !keysLastFrame['S']) {
            sine.start();
            if (!piano.empty()) {
                piano[7]->setColor(pressFill);
            }
        } else if(!keys['S'] && keysLastFrame['S']) {
            sine.stop();
            // Reset color
            if (!piano.empty()) {
                piano[7]->setColor(blackKey);
            }
        }
    }

    if (screen == freePlay) {
        if (keys['D'] && !keysLastFrame['D']) {
            sine.start();
            if (!piano.empty()) {
                piano[8]->setColor(pressFill);
            }
        } else if(!keys['D'] && keysLastFrame['D']) {
            sine.stop();
            // Reset color
            if (!piano.empty()) {
                piano[8]->setColor(blackKey);
            }
        }
    }

    if (screen == freePlay) {
        if (keys['G'] && !keysLastFrame['G']) {
            sine.start();
            if (!piano.empty()) {
                piano[9]->setColor(pressFill);
            }
        } else if(!keys['G'] && keysLastFrame['G']) {
            sine.stop();
            // Reset color
            if (!piano.empty()) {
                piano[9]->setColor(blackKey);
            }
        }
    }

    if (screen == freePlay) {
        if (keys['H'] && !keysLastFrame['H']) {
            sine.start();
            if (!piano.empty()) {
                piano[10]->setColor(pressFill);
            }
        } else if(!keys['H'] && keysLastFrame['H']) {
            sine.stop();
            // Reset color
            if (!piano.empty()) {
                piano[10]->setColor(blackKey);
            }
        }
    }

    if (screen == freePlay) {
        if (keys['J'] && !keysLastFrame['J']) {
            sine.start();
            if (!piano.empty()) {
                piano[11]->setColor(pressFill);
            }
        } else if(!keys['J'] && keysLastFrame['J']) {
            sine.stop();
            // Reset color
            if (!piano.empty()) {
                piano[11]->setColor(blackKey);
            }
        }
    }


    // game play key sounds
    ////// EACH PIANO KEY IS REPRESENTED BY A KEY ON THE KEYBOARD //////

//// WHITE KEYS ////

    if (screen == gamePlay) {
        if (keys['Z'] && !keysLastFrame['Z']) {
            sine.start();
//            sounds[0].makeSine(460.0f);
            // Highlight C key when pressed
            if (!piano.empty()) {
                piano[0]->setColor(pressFill);
            }
        } else if(!keys['Z'] && keysLastFrame['Z']) {
            sine.stop();
            //sound_engine.stopSine(340.0);
            //sounds[0].stopSine(460.0f);
            // Reset color
            if (!piano.empty()) {
                piano[0]->setColor(whiteKey);
            }
        }
    }

    if (screen == gamePlay) {
        if (keys['X'] && !keysLastFrame['X']) {
            sine.start();
//            sounds[0].makeSine(460.0f);
            // Highlight C key when pressed
            if (!piano.empty()) {
                piano[1]->setColor(pressFill);
            }
        } else if(!keys['X'] && keysLastFrame['X']) {
            sine.stop();
            //sound_engine.stopSine(340.0);
            //sounds[0].stopSine(460.0f);
            // Reset color
            if (!piano.empty()) {
                piano[1]->setColor(whiteKey);
            }
        }
    }

    if (screen == gamePlay) {
        if (keys['C'] && !keysLastFrame['C']) {
            sine.start();
            // Highlight C key when pressed
            if (!piano.empty()) {
                piano[2]->setColor(pressFill);
            }
        } else if(!keys['C'] && keysLastFrame['C']) {
            sine.stop();
            //sound_engine.stopSine(340.0);
            //sounds[0].stopSine(460.0f);
            // Reset color
            if (!piano.empty()) {
                piano[2]->setColor(whiteKey);
            }
        }
    }

    if (screen == gamePlay) {
        if (keys['V'] && !keysLastFrame['V']) {
            sine.start();
            if (!piano.empty()) {
                piano[3]->setColor(pressFill);
            }
        } else if(!keys['V'] && keysLastFrame['V']) {
            sine.stop();
            // Reset color
            if (!piano.empty()) {
                piano[3]->setColor(whiteKey);
            }
        }
    }

    if (screen == gamePlay) {
        if (keys['B'] && !keysLastFrame['B']) {
            sine.start();
            if (!piano.empty()) {
                piano[4]->setColor(pressFill);
            }
        } else if(!keys['B'] && keysLastFrame['B']) {
            sine.stop();
            // Reset color
            if (!piano.empty()) {
                piano[4]->setColor(whiteKey);
            }
        }
    }

    if (screen == gamePlay) {
        if (keys['N'] && !keysLastFrame['N']) {
            sine.start();
            if (!piano.empty()) {
                piano[5]->setColor(pressFill);
            }
        } else if(!keys['N'] && keysLastFrame['N']) {
            sine.stop();
            // Reset color
            if (!piano.empty()) {
                piano[5]->setColor(whiteKey);
            }
        }
    }

    if (screen == gamePlay) {
        if (keys['M'] && !keysLastFrame['M']) {
            sine.start();
            if (!piano.empty()) {
                piano[6]->setColor(pressFill);
            }
        } else if(!keys['M'] && keysLastFrame['M']) {
            sine.stop();
            // Reset color
            if (!piano.empty()) {
                piano[6]->setColor(whiteKey);
            }
        }
    }

    //// Black KEYS ////

    if (screen == gamePlay) {
        if (keys['S'] && !keysLastFrame['S']) {
            sine.start();
            if (!piano.empty()) {
                piano[7]->setColor(pressFill);
            }
        } else if(!keys['S'] && keysLastFrame['S']) {
            sine.stop();
            // Reset color
            if (!piano.empty()) {
                piano[7]->setColor(blackKey);
            }
        }
    }

    if (screen == gamePlay) {
        if (keys['D'] && !keysLastFrame['D']) {
            sine.start();
            if (!piano.empty()) {
                piano[8]->setColor(pressFill);
            }
        } else if(!keys['D'] && keysLastFrame['D']) {
            sine.stop();
            // Reset color
            if (!piano.empty()) {
                piano[8]->setColor(blackKey);
            }
        }
    }

    if (screen == gamePlay) {
        if (keys['G'] && !keysLastFrame['G']) {
            sine.start();
            if (!piano.empty()) {
                piano[9]->setColor(pressFill);
            }
        } else if(!keys['G'] && keysLastFrame['G']) {
            sine.stop();
            // Reset color
            if (!piano.empty()) {
                piano[9]->setColor(blackKey);
            }
        }
    }

    if (screen == gamePlay) {
        if (keys['H'] && !keysLastFrame['H']) {
            sine.start();
            if (!piano.empty()) {
                piano[10]->setColor(pressFill);
            }
        } else if(!keys['H'] && keysLastFrame['H']) {
            sine.stop();
            // Reset color
            if (!piano.empty()) {
                piano[10]->setColor(blackKey);
            }
        }
    }

    if (screen == gamePlay) {
        if (keys['J'] && !keysLastFrame['J']) {
            sine.start();
            if (!piano.empty()) {
                piano[11]->setColor(pressFill);
            }
        } else if(!keys['J'] && keysLastFrame['J']) {
            sine.stop();
            // Reset color
            if (!piano.empty()) {
                piano[11]->setColor(blackKey);
            }
        }
    } // end game play key sounds

    if(screen == gamePlay) {
        // if mouse pressed now and not pressed last frame
        // make sound
        if (keyOverlapsMouse && mousePressed && !mousePressedLastFrame) {
            sound_engine.makeSine(440);
        }
        // if mouse pressed now false and it was pressed last frame
        // stop sound
        if (keyOverlapsMouse && !mousePressed && mousePressedLastFrame) {
            sound_engine.stopSine(440);
        }
    }

    if(screen == freePlay) {
        // if mousepressed now and not pressed last frame
        // make sound
        if (keyOverlapsMouse && mousePressed && !mousePressedLastFrame) {
            sound_engine.makeSine(880);
        }
        // if mousepressed now false and it was pressed last frame
        // stop sound
        if (keyOverlapsMouse && !mousePressed && mousePressedLastFrame) {
            sound_engine.stopSine(880);
        }
    }

    // Save mousePressed for next frame
    mousePressedLastFrame = mousePressed;

    for (int key = 0; key < 1024; ++key) {
        keysLastFrame[key] = keys[key];
    }

}

void Engine::update() {
    // Calculate delta time
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    bool playedRight = false;

//    for (int i = 0; i < piano.size(); ++i) {
//        if (playSound[i]) {
//            // Start the sine wave associated with the pressed key
//            sound_engine.makeSine(frequencies[i]);
//        } else {
//            // Stop the sine wave associated with the released key
//            sound_engine.stopSine(frequencies[i]);
//        }
//    }

    // TODO: When in gamePlay mode, end the game when the user correctly plays the song
    if(screen == gamePlay && playedRight){
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
            this->fontRenderer->renderText(title, width / 2 - (20 * title.length()), height / 1.25, 1.75,
                                           vec3{1, 1, 1});
            // Each sentence
            string sentence1 = "Welcome to our interactive piano practice program!";
            string sentence2 = "Play for fun or practice a short song";
            //string sentence3 = "Switch off all the lights with the least number of clicks.";
            // Positioning, size, color
            this->fontRenderer->renderText(sentence1, width / 6.5 - (5 * title.length()), height / 2 + 50, 0.58,
                                           vec3{0.808, 0.396, 0.667});
            this->fontRenderer->renderText(sentence2, width / 6.5 - (5 * title.length()), height / 2, 0.58,
                                           vec3{0.808, 0.396, 0.667});
            //this->fontRenderer->renderText(sentence3, width / 6.5 - (5 * title.length()), height / 2 - 50, 0.5,vec3{0.871, 0.055, 0.8});
            string fun = "Press S to play for fun";
            // (12 * message.length()) is the offset to center text.
            // 12 pixels is the width of each character scaled by 1.
            this->fontRenderer->renderText(fun, width / 2 - (12 * fun.length()), height / 5, 0.9,
                                           vec3{0.604, 0.325, 0.6});
            string practice = "Press P to practice a song";
            // (12 * message.length()) is the offset to center text.
            // 12 pixels is the width of each character scaled by 1.
            this->fontRenderer->renderText(practice, width / 2 - (12 * practice.length()), height / 5 + 50, 0.9,
                                           vec3{0.604, 0.325, 0.6});
            // Reset elapsedTime every time user is on start screen
            elapsedTime = 0.0f;

            if (!isPlaying) {
                sine.start();
            }

            break;
        }

        case freePlay: {
            // Check if 5 seconds have passed to hide the text
            if (elapsedTime < 1.0f) { // TODO: change this back to 5.0 after testing
                glClearColor(0.596f, 0.714f, 0.929f, 1.0f); // Light blue background
                // Clear the color buffer
                glClear(GL_COLOR_BUFFER_BIT);
                string title = "How to play";
                // Displayed at top of screen
                this->fontRenderer->renderText(title, width / 1.8 - (20 * title.length()), height / 1.2, 1.5, vec3{0.996, 0.796, 0.243});
                int leftAlign = 50;
                int verticalSpacing = 40;
                string i1 = ">> Simply click on the piano keys to produce sound";
                this->fontRenderer->renderText(i1, leftAlign, height - 230, 0.60, vec3{0.984, 0.945, 0.933});

                string i2 = ">> Press the left arrow key to return home";
                this->fontRenderer->renderText(i2, leftAlign, height - 230 - verticalSpacing, 0.60, vec3{0.984, 0.945, 0.933});

                string i3 = ">> Press esc to exit";
                this->fontRenderer->renderText(i3, leftAlign, height - 230 - 2 * verticalSpacing, 0.60, vec3{0.984, 0.945, 0.933});

            } else {
                showText = false;
            }
            // Increment the elapsed time
            elapsedTime += deltaTime; // deltaTime is the time since the last frame

            // Main background
            glClearColor(0.686f, 0.816f, 1.0f, 1.0f);  // blue background
            glClear(GL_COLOR_BUFFER_BIT);

            // Draw piano
            for(const unique_ptr<Shape>& r: piano){
                r->setUniforms();
                r->draw();
            }

            break;
        }
            //Add a practice screen here
        case gamePlay: {
            // Check if 5 seconds have passed to hide the text
            if (elapsedTime < 1.0f) { // TODO: change this back to 10.0 after testing
                glClearColor(0.596f, 0.714f, 0.929f, 1.0f); // Light blue background
                // Clear the color buffer
                glClear(GL_COLOR_BUFFER_BIT);
                string title = "How to practice";
                // Displayed at top of screen
                this->fontRenderer->renderText(title, width / 1.8 - (20 * title.length()), height / 1.2, 1.5, vec3{0.996, 0.796, 0.243});
                int leftAlign = 50;
                int initialVerticalPosition = height - 160;
                int verticalSpacing = 40;
                string i1 = ">> The program will play the song first and ";
                this->fontRenderer->renderText(i1, leftAlign, initialVerticalPosition, 0.60, vec3{0.984, 0.945, 0.933});
                string i2 = "   highlight each key played on the keyboard";
                this->fontRenderer->renderText(i2, leftAlign, initialVerticalPosition - verticalSpacing, 0.60, vec3{0.984, 0.945, 0.933});
                string i3 = ">> Now, it's your turn! Click on the";
                this->fontRenderer->renderText(i3, leftAlign, initialVerticalPosition - 2 * verticalSpacing, 0.60, vec3{0.984, 0.945, 0.933});
                string i4 = "    highlighted keys as they appear";
                this->fontRenderer->renderText(i4, leftAlign, initialVerticalPosition - 3 * verticalSpacing, 0.60, vec3{0.984, 0.945, 0.933});
                string i5 = ">> Once you're done playing, the program will ";
                this->fontRenderer->renderText(i5, leftAlign, initialVerticalPosition - 4 * verticalSpacing, 0.60, vec3{0.984, 0.945, 0.933});
                string i6 = "   play the song one more time";
                this->fontRenderer->renderText(i6, leftAlign, initialVerticalPosition - 5 * verticalSpacing, 0.60, vec3{0.984, 0.945, 0.933});
                string i7 = ">> It's game time now! Play the song correctly";
                this->fontRenderer->renderText(i7, leftAlign, initialVerticalPosition - 6 * verticalSpacing, 0.60, vec3{0.984, 0.945, 0.933});
                string i8 = "   to win";
                this->fontRenderer->renderText(i8, leftAlign, initialVerticalPosition - 7 * verticalSpacing, 0.60, vec3{0.984, 0.945, 0.933});
                string i9 = ">> Press the left arrow key to return home";
                this->fontRenderer->renderText(i9, leftAlign, initialVerticalPosition - 8 * verticalSpacing, 0.60, vec3{0.984, 0.945, 0.933});
                string i10 = ">> Press esc to exit";
                this->fontRenderer->renderText(i10, leftAlign, initialVerticalPosition - 9 * verticalSpacing, 0.60, vec3{0.984, 0.945, 0.933});

            } else {
                showText = false;
            }
            // Increment the elapsed time
            elapsedTime += deltaTime; // deltaTime is the time since the last frame

            // Main background
            glClearColor(0.686f, 0.816f, 1.0f, 1.0f);  // blue background
            glClear(GL_COLOR_BUFFER_BIT);

            // Draw piano
            for(const unique_ptr<Shape>& r: piano){
                r->setUniforms();
                r->draw();
            }

            //// GAME LOGIC ////

            // Check if elapsedTime is within the 2 sec buffer time + the time to play the sound (1 sec)
            if (elapsedTime > 2.0f && elapsedTime < 3.0f) {
                // Start playing the sound and change color
                sine.start();
                if (!piano.empty()) {
                    piano[0]->setColor(pressFill);
                }
            } else {
                // Stop the sound and reset color
                sine.stop();
                if (!piano.empty()) {
                    piano[0]->setColor(whiteKey);
                }
            }


            break;
        }


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

//void Engine::spawnConfetti() {
//    vec2 pos = {rand() % (int)width, rand() % (int)height};
//    int rd = 1 + (rand() % 100);
//    vec2 size = {rd, rd}; // placeholder
//    color color = {float(rand() % 10 / 10.0), float(rand() % 10 / 10.0), float(rand() % 10 / 10.0), 1.0f};
//    confetti.push_back(make_unique<Rect>(shapeShader, pos, size, color));
//}

void Engine::resetKeyColor(int key) {
    // Determine the original color of the button
    color originalColor = keyVec[key]->getColor();

    // Check if the original color was black or white
    if (originalColor.red == 0 && originalColor.green == 0 && originalColor.blue == 0) {
        // If original color was black, reset to black
        keyVec[key]->setColor(color{0, 0, 0, 1});
    } else {
        // Otherwise, reset to white
        keyVec[key]->setColor(color{1, 1, 1, 1});
    }
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
