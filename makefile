
includedir = $(shell llvm-config-21 --includedir)
libs = $(shell llvm-config-21 --link-static --ldflags --libs core passes)
PluginLoads = -load-pass-plugin ./llvm-tutor-testing/build/lib/libRIV.so -load-pass-plugin=./out/libMergeB.so -load-pass-plugin=./out/libDuplicateB.so
test: mkdir build
	@cp ./strLen.ll ./tmp/input_for_passes.ll
	@-echo -e running RIV.so plugin on input_for_passes.ll\n
	@opt $(PluginLoads) -passes="print<riv>" ./tmp/input_for_passes.ll -S -o ./tmp/output_from_duplicate.ll
	@-echo -e \n\nrunning DuplicateBB.so plugin on input_for_passes.ll
	@opt $(PluginLoads) -passes=duplicate-b ./tmp/input_for_passes.ll -S -o ./tmp/output_from_duplicate.ll
	@-echo running MergeB.so plugin on output_from_duplicate.ll
	@opt $(PluginLoads) -passes=merge-b ./tmp/output_from_duplicate.ll -S -o ./tmp/output_from_merge.ll
.phony : test

build: mkdir ./src/MergeB.cpp
	@-echo building libDuplicateB.so
	@clang++ -fPIC -Wall -Wno-unused-command-line-argument -Wno-deprecated-declarations -Werror -fdeclspec -std=c++23 -O3 $(libs) -I$(includedir) -I./include ./src/DuplicateB.cpp -shared -o ./out/libDuplicateB.so
	@-echo finished building libDuplicateB.so
	@-echo building libMergeB.so
	@clang++ -fPIC -Wall -Wno-unused-command-line-argument -Wno-deprecated-declarations -Werror -fdeclspec -std=c++23 -O3 $(libs) -I$(includedir) -I./include ./src/MergeB.cpp -shared -o ./out/libMergeB.so
	@-echo finished building libMergeB.so
.phony : build
mkdir:
	@-rm -rf out
	@-rm -rf tmp
	@mkdir out
	@mkdir tmp
.phony : mkdir

