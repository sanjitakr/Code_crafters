To run the code from Projectory directory:


sudo apt install gcc make pkg-config libwebsockets-dev libjansson-dev

OR 


sudo dnf install gcc make pkgconf-pkg-config libwebsockets-devel jansson-devel

<pre> bash gcc server.c -o server -pthread $(pkg-config --cflags libwebsockets jansson) $(pkg-config --libs libwebsockets jansson) -ljansson</pre>

Then close this terminal. Open a new terminal and call ./server

Then go to the folder and open index.html.


Open index.html in multiple browsers to represent different clients. Each client will be prompted for username/password (alice/1234 or bob/abcd have been set as default in the code just for testing)


The code uses html,css and js for the frontend and the server and client run on C in the backend. The program allows users to work on a project at the same time and view what is being selected. They can also change the colour of the font of the text as well as the bold, underlin, italics feature making it a rich text editor. When the index.html is closed and opened later the previous text is saved.
