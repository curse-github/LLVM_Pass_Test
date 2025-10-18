
includedir = $(shell llvm-config-21 --includedir)
libs = $(shell llvm-config-21 --link-static --ldflags --libs core passes)
test: mkdir build lib
	@cp ./strLen.ll ./tmp/input_for_passes.ll
	@-echo -e running RIV.so plugin on input_for_passes.ll
	@opt -load-pass-plugin=./llvm-tutor-testing/build/lib/libRIV.so -p="print<riv>" ./tmp/input_for_passes.ll -disable-output
	@-echo running DuplicateB.so plugin on input_for_passes.ll
	@opt -load-pass-plugin=./llvm-tutor-testing/build/lib/libRIV.so -load-pass-plugin=./out/libDuplicateB.so -p=duplicate-b ./tmp/input_for_passes.ll -S -o ./tmp/output_from_duplicate.ll
	@-echo running MergeB.so plugin on output_from_duplicate.ll
	@opt -load-pass-plugin=./out/libMergeB.so -p=merge-b ./tmp/output_from_duplicate.ll -S -o ./tmp/output_from_merge.ll

	@clang++ -Werror -Wno-override-module -std=c++23 -O3 ./tmp/input_for_passes.ll ./out/libStd.a -o ./out/input.out
	@clang++ -Werror -Wno-override-module -std=c++23 -O3 ./tmp/output_from_duplicate.ll ./out/libStd.a -o ./out/output_d.out
	@clang++ -Werror -Wno-override-module -std=c++23 -O3 ./tmp/output_from_merge.ll ./out/libStd.a -o ./out/output_m.out

	@-echo testing input.out
	@-./out/input.out
	@-./out/input.out ""
	@-./out/input.out "abcdefghijklmnopqrstuvwxyz"
	
	@-echo testing output_d.out
	@-./out/output_d.out
	@-./out/output_d.out ""
	@-./out/output_d.out "abcdefghijklmnopqrstuvwxyz"
	
	@-echo testing output_m.out
	@-./out/output_m.out
	@-./out/output_m.out ""
	@-./out/output_m.out "abcdefghijklmnopqrstuvwxyz"
.phony : test
lib: mkdir ./lib/cppStdLib.cpp ./lib/llvmStdLib.ll
	@-echo building std lib
	@clang++ -Werror -Wall -O3 ./lib/cppStdLib.cpp -c -o ./tmp/cppStdLib.o
	@clang++ -Werror -Wall -O3 ./lib/llvmStdLib.ll -c -o ./tmp/llvmStdLib.o
	@ar rcs ./out/libStd.a ./tmp/cppStdLib.o ./tmp/llvmStdLib.o
	@-echo finished building std lib
.phony : lib

build: mkdir ./src/MergeB.cpp
	@-echo building libDuplicateB.so
	@clang++ -fPIC -Werror -Wall -Wno-unused-command-line-argument -Wno-deprecated-declarations -fdeclspec -std=c++23 -O3 $(libs) -I$(includedir) -I./include ./src/DuplicateB.cpp -shared -o ./out/libDuplicateB.so
	@-echo finished building libDuplicateB.so
	@-echo building libMergeB.so
	@clang++ -fPIC -Werror -Wall -Wno-unused-command-line-argument -Wno-deprecated-declarations -Wno-unused-variable -fdeclspec -std=c++23 -O3 $(libs) -I$(includedir) -I./include ./src/MergeB.cpp -shared -o ./out/libMergeB.so
	@-echo finished building libMergeB.so
.phony : build
mkdir:
	@-rm -rf out
	@-rm -rf tmp
	@mkdir out
	@mkdir tmp
.phony : mkdir

