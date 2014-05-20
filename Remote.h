
#ifndef __REMOTE_H_
#define __REMOTE_H_

#include "Manager.h"

class WindowManager;

class Connection {
 public:
    Connection(int, int);
    ~Connection();
    int store_input(class Remote*);

 private:
    int fd;	// for debugging
    char *buff;
    int size;	// size allocated
    int room;	// room left
    char *p;	// next input goes here
};

class Remote {
 public:
    Remote(WindowManager *const);
    ~Remote();
    void setup_port(char *listenon, int port, int verbose);
    int RemoteControlIsOn(void) { return remote_control_port; }
    Boolean isRemoteControlFd(int fd);
    Boolean doRemoteControl(int fd);
    void closeConnection(int);
    int parse_command(char *buff, int fd);
    int	verbose; 
    void close_all_sockets(void);

 private:

    WindowManager *wm;
    int remote_control_port;
    fd_set remote_control_connections;
    int remote_control_socket;
    int setup_socket(char *host, int port);

    int get_connection(int socket_fd);

    int max_connection;	// 1 + highest fd in connections
    Connection **connections;

    // why is this static?  (copied from Manager.h)
    static void sigpipe_handler(int);


};

#endif
