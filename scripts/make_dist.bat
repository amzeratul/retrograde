cd ..
rd /s /q retrograde
mkdir retrograde
mkdir retrograde\bin
mkdir retrograde\roms
mkdir retrograde\cores
mkdir retrograde\profiles
copy bin\retrograde.exe retrograde\bin\retrograde.exe /Y
copy bin\SDL2.dll retrograde\bin\SDL2.dll /Y
robocopy config retrograde\config /E
robocopy images retrograde\images /E
robocopy shaders retrograde\shaders /E
robocopy assets retrograde\assets /E