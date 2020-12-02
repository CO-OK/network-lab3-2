server: dgram.c lab3-2-server.c msghdr.h window.c window.h
		gcc dgram.c lab3-2-server.c msghdr.h window.c window.h -g -lpthread -o ./out/server
client: dgram.c lab3-2-client.c msghdr.h window.c window.h
		gcc dgram.c lab3-2-client.c msghdr.h window.c window.h -g -lpthread -o ./out/client
clean:
		rm ./out/client ./out/server 