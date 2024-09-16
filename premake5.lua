workspace "MetalRenderer"
    configurations { "Debug", "Release" }
    platforms { "x86_64", "arm64" }
    location "build"
    targetdir "bin/%{cfg.buildcfg}"
    objdir "obj/%{cfg.buildcfg}"

    filter "configurations:Debug"
        symbols "On"
        defines { "DEBUG" }
        optimize "Off"
    filter "configurations:Release"
        symbols "Off"
        defines { "NDEBUG" }
        optimize "Full"
    filter "platforms:x86_64"
        architecture "x86_64"
    filter "platforms:arm64"
        architecture "ARM64"
    filter {}

project "MetalRenderer"
    kind "ConsoleApp"
    language "C++"
    -- compileas "Objective-C++"
    cppdialect "C++17"
    files { "src/**.h", "src/**.cpp", "src/**.c", "src/**.m", "src/**.mm", "lib/**" }
    includedirs { "lib/metalcpp", "lib", "lib/imgui" }
    -- /opt/homebrew/Cellar/sdl2/2.30.7
    includedirs { "/opt/homebrew/Cellar/sdl2/2.30.7/include", "/opt/homebrew/Cellar/sdl2/2.30.7/include/SDL2", "/opt/homebrew/Cellar/sdl2_image/2.8.2_2/include", "/opt/homebrew/Cellar/glm/1.0.1/include", "/opt/homebrew/Cellar/freetype/2.13.3/include/freetype2" }
    libdirs { "/opt/homebrew/Cellar/sdl2/2.30.7/lib", "/opt/homebrew/Cellar/sdl2_image/2.8.2_2/lib", "/opt/homebrew/Cellar/glm/1.0.1/lib", "/opt/homebrew/Cellar/freetype/2.13.3/lib" }
    links { "SDL2", "SDL2_image", "glm", "freetype" }

    includedirs { "src/**" }

    links { "Foundation.framework", "QuartzCore.framework", "Metal.framework" }
    linkoptions { "-framework Foundation", "-framework QuartzCore", "-framework Metal" }

    filter "platforms:x86_64"
        architecture "x86_64"
    filter "platforms:arm64"
        architecture "ARM64"
    filter {}
