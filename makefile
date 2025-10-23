ifeq ($(OS),Windows_NT)
	objectExt = obj
	staticExt = lib
	dynamicExt = dll
	executableExt = exe
else
	objectExt = o
	staticExt = a
	dynamicExt = so
	executableExt = out
endif

includedir = $(shell llvm-config --includedir)
libs = $(shell llvm-config --link-static --ldflags --libs core passes)
test: mkdir build lib
	@cp ./strLen.ll ./tmp/input_for_passes.ll
	@-echo running DuplicateB.$(dynamicExt) plugin on input_for_passes.ll
	@opt -load-pass-plugin=./out/libRIV.$(dynamicExt) -load-pass-plugin=./out/libDuplicateB.$(dynamicExt) -p=duplicate-b,duplicate-b,duplicate-b ./tmp/input_for_passes.ll -S -o ./tmp/output_from_duplicate.ll
	@-echo running MergeB.$(dynamicExt) plugin on output_from_duplicate.ll
	@opt -load-pass-plugin=./out/libMergeB.$(dynamicExt) -p=merge-b ./tmp/output_from_duplicate.ll -S -o ./tmp/output_from_merge.ll

	@clang++ -Werror -Wno-override-module -std=c++23 -O3 ./tmp/input_for_passes.ll ./out/libStd.$(staticExt) -o ./out/input.$(executableExt)
	@clang++ -Werror -Wno-override-module -std=c++23 -O3 ./tmp/output_from_duplicate.ll ./out/libStd.$(staticExt) -o ./out/output_d.$(executableExt)
	@clang++ -Werror -Wno-override-module -std=c++23 -O3 ./tmp/output_from_merge.ll ./out/libStd.$(staticExt) -o ./out/output_m.$(executableExt)

	@-echo testing input.$(executableExt)
	@-./out/input.$(executableExt)
	@-./out/input.$(executableExt) ""
	@-./out/input.$(executableExt) "abcdefghijklmnopqrstuvwxyz"
	
	@-echo testing output_d.$(executableExt)
	@-./out/output_d.$(executableExt)
	@-./out/output_d.$(executableExt) ""
	@-./out/output_d.$(executableExt) "abcdefghijklmnopqrstuvwxyz"
	
	@-echo testing output_m.$(executableExt)
	@-./out/output_m.$(executableExt)
	@-./out/output_m.$(executableExt) ""
	@-./out/output_m.$(executableExt) "abcdefghijklmnopqrstuvwxyz"
.phony : test
lib: mkdir ./lib/cppStdLib.cpp ./lib/llvmStdLib.ll
	@-echo building std lib
	@clang++ -Werror -Wall -O3 ./lib/cppStdLib.cpp -c -o ./tmp/cppStdLib.$(objectExt)
	@clang++ -Werror -Wall -O3 ./lib/llvmStdLib.ll -c -o ./tmp/llvmStdLib.$(objectExt)
	@ar rcs ./out/libStd.$(staticExt) ./tmp/cppStdLib.$(objectExt) ./tmp/llvmStdLib.$(objectExt)
	@-echo finished building std lib
.phony : lib

build: mkdir ./src/MergeB.cpp
	@-echo building libRIV.$(dynamicExt)
	@clang++ -fPIC -Werror -Wall -Wno-unused-command-line-argument -Wno-deprecated-declarations -fdeclspec -std=c++23 -O3 $(libs) -I$(includedir) -I./include ./src/RIV.cpp -shared -o ./out/libRIV.$(dynamicExt)
	@-echo finished building libRIV.$(dynamicExt)
	@-echo building libDuplicateB.$(dynamicExt)
	@clang++ -fPIC -Werror -Wall -Wno-unused-command-line-argument -Wno-deprecated-declarations -fdeclspec -std=c++23 -O3 $(libs) -I$(includedir) -I./include ./src/DuplicateB.cpp -shared -o ./out/libDuplicateB.$(dynamicExt)
	@-echo finished building libDuplicateB.$(dynamicExt)
	@-echo building libMergeB.$(dynamicExt)
	@clang++ -fPIC -Werror -Wall -Wno-unused-command-line-argument -Wno-deprecated-declarations -fdeclspec -std=c++23 -O3 $(libs) -I$(includedir) -I./include ./src/MergeB.cpp -shared -o ./out/libMergeB.$(dynamicExt)
	@-echo finished building libMergeB.$(dynamicExt)
.phony : build
mkdir:
ifeq ($(OS),Windows_NT)
	@-rmdir /s /q out
	@-rmdir /s /q tmp
else
	@-rm -rf out
	@-rm -rf tmp
endif
	@mkdir out
	@mkdir tmp
.phony : mkdir

