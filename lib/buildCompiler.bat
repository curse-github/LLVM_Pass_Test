:: @echo off
del /Q .\out\*
rmdir out
mkdir out
clang++ -O3 -emit-llvm -S ./lib/cppStdLib.cpp -o ./out/cppStdLib.ll
clang++ ./out/cppStdLib.ll -c -o ./out/cppStdLib.obj
clang++ ./lib/stdLibWin.ll -c -o ./out/stdLib.obj
FOR /F "tokens=*" %%g IN ('..\builds\llvm-build\Release\bin\llvm-config.exe --libdir') do (SET libdir=%%g)
FOR /F "tokens=*" %%g IN ('..\builds\llvm-build\Release\bin\llvm-config.exe --link-static --ldflags --libs core bitwriter irreader profiledata object support debuginfodwarf textapi') do (SET libs=%%g)
clang++ -std=c++23 -O3 ./src/compiler.cpp -L%libdir% %libs% -lntdll -I..\builds\llvm-build\include -I.\include -D_WINDOWS=1 -o ./out/compiler.exe
clang++ -std=c++23 -O3 ./src/simpleCompiler.cpp -L%libdir% %libs% -lntdll -I..\builds\llvm-build\include -I.\include -D_WINDOWS=1 -o ./out/simpleCompiler.exe
out\simpleCompiler.exe simpleModule
out\compiler.exe module
clang++ -O3 ./out/module.ll ./out/cppStdLib.obj ./out/stdLib.obj -o ./out/module.exe