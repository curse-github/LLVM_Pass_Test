LLVM_DIR="$(llvm-config --libdir)/.."
TUTOR_DIR="$PWD/llvm-tutor";
if [ ! -d "./build" ] ; then
    git clone https://github.com/banach-space/llvm-tutor.git
    mkdir -p build/HelloWorld
    cd build
    cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR $TUTOR_DIR/
    cmake --build .
    cd HelloWorld
    cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR $TUTOR_DIR/HelloWorld/
    cmake --build .
fi