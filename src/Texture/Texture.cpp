#include "Texture.hpp"

Texture::Texture(const char *filepath, MTL::Device *metalDevice)
    : device(metalDevice)
{
    SDL_Surface *image = IMG_Load(filepath);

    if (!image)
    {
        std::cerr << "IMG_Load Error: " << IMG_GetError() << std::endl;
        return;
    }

    SDL_Surface *convertedImage = SDL_ConvertSurfaceFormat(image, SDL_PIXELFORMAT_ABGR8888, 0);
    if (!convertedImage)
    {
        std::cerr << "SDL_ConvertSurfaceFormat Error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(image);
        return;
    }

    width = convertedImage->w;
    height = convertedImage->h;
    channels = convertedImage->format->BytesPerPixel;

    size_t dataSize = width * height * 4;
    unsigned char *flippedPixels = new unsigned char[dataSize];
    for (int y = 0; y < height; ++y)
    {
        memcpy(flippedPixels + (height - 1 - y) * width * 4,
               static_cast<unsigned char *>(convertedImage->pixels) + y * convertedImage->pitch,
               width * 4);
    }

    MTL::TextureDescriptor *textureDescriptor = MTL::TextureDescriptor::alloc()->init();
    textureDescriptor->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
    textureDescriptor->setWidth(width);
    textureDescriptor->setHeight(height);
    textureDescriptor->setTextureType(MTL::TextureType2D);
    textureDescriptor->setUsage(MTL::TextureUsageShaderRead);

    texture = device->newTexture(textureDescriptor);
    if (!texture)
    {
        std::cerr << "Failed to create Metal texture." << std::endl;
        delete[] flippedPixels;
        SDL_FreeSurface(convertedImage);
        SDL_FreeSurface(image);
        textureDescriptor->release();
        return;
    }

    MTL::Region region = MTL::Region::Make2D(0, 0, width, height);
    NS::UInteger bytesPerRow = 4 * width;

    texture->replaceRegion(region, 0, flippedPixels, bytesPerRow);

    delete[] flippedPixels;
    SDL_FreeSurface(convertedImage);
    SDL_FreeSurface(image);
    textureDescriptor->release();

    std::cout << "Texture loaded successfully: " << filepath << std::endl;
}

Texture::~Texture()
{
    if (texture)
    {
        texture->release();
        texture = nullptr;
    }
}
