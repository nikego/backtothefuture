# backtothefuture

example of cmd file to build:

---

@echo off

rem subjects to change

set gen="Visual Studio 17 2022" -A x64 -DCMAKE_GENERATOR_TOOLSET=v140

set boost_dir=C:/boost/include/boost-1_80

set zstd_dir=C:/zstd

if not exist bin (
   mkdir bin
)

cd bin

cmake ../src -G %gen% -DBoost_NO_WARN_NEW_VERSIONS=1 -DBOOST_INCLUDEDIR=%boost_dir% -DBOOST_ROOT=%boost_dir%/boost -DZSTD_DIR=%zstd_dir%

cmake --build . --config=Release

if ERRORLEVEL 1  goto exit

echo build OK

%cd%\Release\BackToTheFuture.exe --help

:exit

cd ..


