#run sender, check /dev/shm/, run receiver
gcc -o sender sender.c -lrt
gcc -o receiver receiver.c -lrt

# run worker in background, then run request
gcc -o request request.c -lrt
gcc -o worker worker.c -lrt
