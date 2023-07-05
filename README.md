This is simple Client-Server programm written on C++.

Server are accepting multiple connections.
Server can send the command to client by Id. You need just write in cmd "sendto:<id>:<message>".

Client will connecting to server. If connection is disconnected, client will try connect to server every 10 seconds.
