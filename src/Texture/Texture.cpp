#include "Texture.hpp"

Texture::Texture(const char *filepath, MTL::Device *metalDevice)
{
    device = metalDevice;

    SDL_Surface *image = IMG_Load(filepath);

    if (!image)
    {
        std::cerr << "IMG_Load Error: " << IMG_GetError() << std::endl;
        return;
    }

    // Ensure the image is in RGBA format
    SDL_Surface *convertedImage = SDL_ConvertSurfaceFormat(image, SDL_PIXELFORMAT_ABGR8888, 0);
    // We use ABGR8888 since SDL's byte order may not match Metal's expectation.

    if (!convertedImage)
    {
        std::cerr << "SDL_ConvertSurfaceFormat Error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(image);
        return;
    }

    width = convertedImage->w;
    height = convertedImage->h;
    channels = convertedImage->format->BytesPerPixel;

    // Manually flip the image data vertically
    unsigned char *flippedPixels = new unsigned char[width * height * 4]; // 4 bytes per pixel (ABGR)
    for (int y = 0; y < height; ++y)
    {
        memcpy(flippedPixels + (height - 1 - y) * width * 4,
               (unsigned char *)convertedImage->pixels + y * convertedImage->pitch,
               width * 4);
    }

    // Create Metal texture descriptor
    MTL::TextureDescriptor *textureDescriptor = MTL::TextureDescriptor::alloc()->init();
    textureDescriptor->setPixelFormat(MTL::PixelFormatRGBA8Unorm); // Metal expects RGBA
    textureDescriptor->setWidth(width);
    textureDescriptor->setHeight(height);

    // Create Metal texture
    texture = device->newTexture(textureDescriptor);

    // Upload texture data to Metal texture
    MTL::Region region = MTL::Region(0, 0, 0, width, height, 1);
    NS::UInteger bytesPerRow = 4 * width;

    texture->replaceRegion(region, 0, flippedPixels, bytesPerRow);

    delete[] flippedPixels; // Clean up the flipped data
    SDL_FreeSurface(convertedImage);
    SDL_FreeSurface(image);

    std::cout << "Texture loaded successfully." << std::endl;
}

Texture::~Texture()
{
    texture->release();
}
