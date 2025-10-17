bash build-llvm-tutor.sh
LLVM_DIR="$(llvm-config-21 --libdir)/.."
TUTOR_DIR="$PWD/llvm-tutor";
rm -rf tmp
rm -rf out
mkdir tmp
mkdir out

# for HelloWorld, and InjectFuncCall
clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm $TUTOR_DIR/inputs/input_for_hello.c -o ./tmp/input_for_hello.ll
# for OpcodeCounter, StaticCallCounter, and DynamicCallCounter
clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm $TUTOR_DIR/inputs/input_for_cc.c -o ./tmp/input_for_cc.ll
# for MBASub
clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm $TUTOR_DIR/inputs/input_for_mba_sub.c -o ./tmp/input_for_mba_sub.ll
# for MBAAdd
clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm $TUTOR_DIR/inputs/input_for_mba.c -o ./tmp/input_for_mba.ll
# for RIV
clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm $TUTOR_DIR/inputs/input_for_riv.c -o ./tmp/input_for_riv.ll
# for DuplicateBB
clang -O1 -S -emit-llvm $TUTOR_DIR/inputs/input_for_duplicate_bb.c -o ./tmp/input_for_duplicate_bb.ll
# for FindFCmpEq, and ConvertFCmpEq
clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm $TUTOR_DIR/inputs/input_for_fcmp_eq.c -o ./tmp/input_for_fcmp_eq.ll

echo -e "\n\n\nlibHelloWorld:"
echo "------------------------------------------------------"
opt -load-pass-plugin=./build/HelloWorld/libHelloWorld.so -passes=hello-world ./tmp/input_for_hello.ll -disable-output
echo "------------------------------------------------------"
echo -e "\n\n\nlibOpcodeCounter:"
echo "------------------------------------------------------"
opt -load-pass-plugin=./build/lib/libOpcodeCounter.so -passes="print<opcode-counter>" ./tmp/input_for_cc.ll -disable-output
echo "------------------------------------------------------"
echo -e "\n\n\nlibInjectFuncCall:"
echo "------------------------------------------------------"
opt -load-pass-plugin=./build/lib/libInjectFuncCall.so -passes=inject-func-call ./tmp/input_for_hello.ll -S -o ./tmp/output_from_inject.ll
clang ./tmp/output_from_inject.ll -o ./out/output_from_inject.out
./out/output_from_inject.out
echo "------------------------------------------------------"
echo -e "\n\n\nlibStaticCallCounter:"
echo "------------------------------------------------------"
opt -load-pass-plugin=./build/lib/libStaticCallCounter.so -passes="print<static-cc>" ./tmp/input_for_cc.ll -disable-output
echo "------------------------------------------------------"
echo -e "\n\n\nlibDynamicCallCounter:"
echo "------------------------------------------------------"
opt -load-pass-plugin=./build/lib/libDynamicCallCounter.so -passes=dynamic-cc ./tmp/input_for_cc.ll -S -o ./tmp/output_from_cc.ll
clang ./tmp/output_from_cc.ll -o ./out/output_from_cc.out
./out/output_from_cc.out
echo "------------------------------------------------------"
echo -e "\n\n\nlibMBASub:"
echo "------------------------------------------------------"
opt -load-pass-plugin=./build/lib/libMBASub.so -passes=mba-sub -debug-only=mba-sub -stats ./tmp/input_for_mba_sub.ll -S -o ./tmp/output_from_mba_sub.ll
echo "------------------------------------------------------"
echo "libMBAAdd:"
echo "------------------------------------------------------"
opt -load-pass-plugin=./build/lib/libMBAAdd.so -passes=mba-add -debug-only=mba-add -stats ./tmp/input_for_mba.ll -S -o ./tmp/output_from_mba_add.ll
echo "------------------------------------------------------"
echo -e "\n\n\nlibRIV:"
echo "------------------------------------------------------"
opt -load-pass-plugin=./build/lib/libRIV.so -passes="print<riv>" ./tmp/input_for_riv.ll -disable-output
echo "------------------------------------------------------"
echo -e "\n\n\nlibDuplicateBB:"
echo "------------------------------------------------------"
opt -load-pass-plugin=./build/lib/libRIV.so -load-pass-plugin=./build/lib/libDuplicateBB.so -passes=duplicate-bb ./tmp/input_for_duplicate_bb.ll -S -o ./tmp/output_from_duplicate_bb.ll && \
opt -load-pass-plugin=./build/lib/libRIV.so -load-pass-plugin=./build/lib/libDuplicateBB.so -passes=duplicate-bb,duplicate-bb ./tmp/input_for_duplicate_bb.ll -S -o ./tmp/output_from_duplicate_bbbb.ll && \
echo "    Success"
echo "------------------------------------------------------"
echo -e "libMergeBB:"
echo "------------------------------------------------------"
opt -load-pass-plugin=./build/lib/libRIV.so -load-pass-plugin=./build/lib/libMergeBB.so -load-pass-plugin=./build/lib/libDuplicateBB.so -passes=merge-bb ./tmp/output_from_duplicate_bb.ll -S -o ./tmp/output_from_merge_bb.ll && \
opt -load-pass-plugin=./build/lib/libRIV.so -load-pass-plugin=./build/lib/libMergeBB.so -load-pass-plugin=./build/lib/libDuplicateBB.so -passes=merge-bb ./tmp/output_from_duplicate_bbbb.ll -S -o ./tmp/output_from_merge_bbbb.ll && \
echo "    Success"
echo "------------------------------------------------------"
echo -e "\n\n\nlibFindFCmpEq:"
echo "------------------------------------------------------"
opt -load-pass-plugin=./build/lib/libFindFCmpEq.so -passes="print<find-fcmp-eq>" ./tmp/input_for_fcmp_eq.ll -disable-output
echo "------------------------------------------------------"
echo -e "\n\n\nlibConvertFCmpEq:"
echo "------------------------------------------------------"
opt -load-pass-plugin=./build/lib/libFindFCmpEq.so -load-pass-plugin=./build/lib/libConvertFCmpEq.so -passes=convert-fcmp-eq ./tmp/input_for_fcmp_eq.ll -S -o ./tmp/output_from_convert_fcmp_eq.ll && echo "    Success"
echo "------------------------------------------------------"