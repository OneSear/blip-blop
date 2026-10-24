// Process entry point of SDL 2 builds. It lives in the platform layer
// because SDL.h renames main() to SDL_main() on the systems where SDL2main
// provides the real entry point (Windows), and only src/platform may
// include SDL headers.

#include <SDL.h>

#include "bb/platform/entry.hpp"

int main(int argc, char* argv[]) {
    return bbMain(argc, argv);
}
