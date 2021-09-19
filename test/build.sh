#objdump -d tfunc
#export LD_LIBRARY_PATH=.
gcc -fPIC -O3 -c asmpopen.c
gcc -shared asmpopen.o -o libpopen.so
gcc -O3 -o tfunc tfunc.c -ldl
