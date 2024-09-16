#pragma once
#include <Metal/Metal.hpp>
#include <SDL2/SDL_image.h>
#include <iostream>

class Texture
{
public:
    Texture(const char *filepath, MTL::Device *metalDevice);
    ~Texture();
    MTL::Texture *texture;
    int width, height, channels;

private:
    MTL::Device *device;
};
