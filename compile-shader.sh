# Compile a metal shader

xcrun -sdk macosx metal -c "shaders/triangle.metal" -o "shaders/triangle.metal.ir" 
xcrun -sdk macosx metal -c "shaders/geometry.metal" -o "shaders/geometry.metal.ir" 
xcrun -sdk macosx metal -c "shaders/geometry.debug.metal" -o "shaders/geometry.debug.metal.ir" 

xcrun -sdk macosx metallib shaders/triangle.metal.ir shaders/geometry.metal.ir shaders/geometry.debug.metal.ir -o bin/Release/default.metallib

echo "Success: shader compiled"
