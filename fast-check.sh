CXXFLAGS="-mllvm -$1"

cd test/aes
rm -f TestProgram.out
../../cmake-build-debug/bin/clang++ AES.cpp TestProgram.cpp $CXXFLAGS -o TestProgram.out
# ../../cmake-build-debug/bin/clang++ TestProgram.cpp -emit-llvm -S -g -o TestProgram.dbg.ll
# ../../cmake-build-debug/bin/clang++ TestProgram.cpp -emit-llvm -S -o TestProgram.ll
./TestProgram.out flag{s1mpl3_11vm_d3m0} 
