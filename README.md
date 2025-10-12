To run the code from Projectory directory:

gcc server.c -o server -pthread `pkg-config --cflags libwebsockets jansson` `pkg-config --libs libwebsockets jansson` -ljansson


Then open a new terminal and type./server
Open as many client terminals as you want and type ./client



Open index.html in multiple browsers. Each client will be prompted for username/password (alice/1234 or bob/abcd)


