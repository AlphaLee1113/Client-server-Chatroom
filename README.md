This is a prohect in COMP4621: Computer Communication Networks I
which is a client-server application to create a multi party 
chatroom. In the beginning the server side starts and waits for connection requests 
to arrive from clients. When a client starts it requests immediately a connection to 
the server upon which it is required to supply a nickname for the user in the 
chatroom. The nickname is used for registration or login. The server maintains a list 
or (array) of user nicknames, their associated client socket file descriptors and 
their states (online or offline) to drive the communication

A client can send one of several requests to the server which the server interprets 
and responds to appropriately:
 • REGISTER: is sent in response to the server prompting the user to supply a 
  nickname for registration or login.
 • WHO: can be sent by the client to the server to obtain a tab-separated list of 
  nicknames of all the users in the system (except itself), where the state of each 
  user is also labelled.
 • EXIT: can be sent by the client to the server to leave the chatroom

A client can decide to send a broadcast message to all by simply typing the message 
(like in zoom chatroom), or it can decide to send a message to another client 
privately (via the server). 
For this latter, the client must type #<Nickname>: <MSG>, where <Nickname> is the 
nickname of the destination client and <MSG> is the message we want to send 
privately. If the destination user is offline (exited), the server will store this message 
into its message box (for example, a text file). When the client comes back online, it 
can view the message box and read the messages sent from other clients.
![marking 1](https://github.com/AlphaLee1113/Client-server-Chatroom/assets/113546167/ba61b2bb-e63f-4716-996f-e4f3aa8b76f7)
![marking 2](https://github.com/AlphaLee1113/Client-server-Chatroom/assets/113546167/775cca36-21d9-43f2-9c0b-f08d996b4f94)


