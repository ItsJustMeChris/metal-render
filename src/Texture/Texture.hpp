#pragma once
#include <Metal/Metal.hpp>
#include <SDL2/SDL_image.h>
#include <iostream>

class Texture
{
public:
    Texture(const char *filepath, MTL::Device *metalDevice);
    ~Texture();

    MTL::Texture* getMTLTexture() const { return texture; }

private:
    MTL::Device *device;
    MTL::Texture *texture = nullptr;
    int width = 0, height = 0, channels = 0;
};
