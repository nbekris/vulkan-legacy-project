#include "camera.h"

class App
{
public:
    App(int argc, char** argv);
    
    GLFWwindow* GLFW_window;
    VkApp* VK;

    Camera myCamera;
    void updateCamera();
    
    // retired: bool doApiDump = false;
};
