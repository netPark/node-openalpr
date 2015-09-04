git clone https://github.com/openalpr/openalpr.git
cd openalpr
git clone https://github.com/peters/openalpr-windows.git windows
cd windows
git checkout v2.1.0
git submodule update --init --recursive

.\build.ps1 -Configuration Release -Platform x64 -PlatformToolset v120 -CudaGeneration None