
#define DEFAULT_PORT 6999

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>	/* memcpy */
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <ctype.h>

#include "Remote.h"
#include "Manager.h"
#include "Client.h"

Boolean sigpipe_happened = False;

void Remote::sigpipe_handler(int signal)
{
    printf("SIGPIPE!\n");
    sigpipe_happened = True;
}

Connection::Connection(int buff_size, int new_fd) 
{
    size = buff_size;
    fd = new_fd;
    buff = (char *)malloc(size);
    room = size - 1;
    p = buff;
}

Connection::~Connection() {
    // printf("Connection::~Connection fd = %d\n", fd);
    if (buff) {
	free(buff);
    }
}

// This is called when there is input on a remote connection.
// The command is parsed only when a '\n' is seen so that if there
// is a partial read or more than one command is read at once
// things work as expected.
//
int Connection::store_input(Remote *r)
{
    int n;
    char *next;

    // printf("Connection::store_input for fd %d \n", fd);
    // printf("size: %d  room: %d  p: 0x%x %s\n", size, room, p, p);

    n = read(fd, p, room);
    if ( n < 0 && errno == EINTR ) {
	fprintf(stderr, "interrupted system call!\n") ;
	return 0;	// ???
    }
    if ( n <= 0 ) {
	if (r->verbose) {
	    printf("Connection %d has closed\n", fd);
	}
	return -1;
    }
    p[n] = '\0';
    room -= n;
    if (r->verbose) {
	printf("Read %d bytes from fd %d: ", n, fd);
	printf("buff is now: %s", buff);
    }

    while ( (next = strchr(buff, '\n')) ) {
	*next++ = '\0';
	// printf("Found a command: %s\n", buff);
	n = r->parse_command(buff, fd);
	if (n < 0) {
	    return n;
	}
	// for next time:
	strlcpy(buff, next, size);
	room = size - strlen(buff) - 1;
    }
    return 0;
}


Remote::Remote(WindowManager *const w_mgr)
{
    wm = w_mgr;

    verbose = 0;
    FD_ZERO(&remote_control_connections);
    remote_control_port = 0;
    remote_control_socket = -1;
    max_connection = 0;
    connections = NULL;
    signal(SIGPIPE, sigpipe_handler);
}


Remote::~Remote()
{
    // empty
}



/// fd_set	connections;
/// fd_set	fds_for_sigio;
/// int	max_fd = 0;

#define BACKLOG 5


int Remote::setup_socket(char *host, int port)
{
    /* setup a connection to given port and host(name)
     * if ok, return fd, 
     * else return -1
     */
    int	fd_socket, n ;
    struct sockaddr	socket_addr ;	/* general socket */
    struct sockaddr_in	inet_socket_adr ;	/* inet socket */
    struct in_addr	ip_addr ;
    struct hostent	*h ;
    int			opt;


    if ( (h = gethostbyname(host)) == NULL ) {
	perror("host not found") ;
	return(-1) ;
    }
    memcpy(& ip_addr.s_addr,  h->h_addr_list[0],  sizeof(struct in_addr)) ;
    if (verbose) {
	fprintf(stderr, "%x\n", ip_addr.s_addr) ;
    }

    fd_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) ;
    if (fd_socket < 0) {
	perror("socket") ;
	return(-1) ;
    }
    inet_socket_adr.sin_family = AF_INET ;
    inet_socket_adr.sin_port = htons(port) ;
    inet_socket_adr.sin_addr = ip_addr ;
    memcpy( (char*) &socket_addr, (char *) &inet_socket_adr, 
	    sizeof(socket_addr)) ;

    opt = 1 ;
    n = setsockopt(fd_socket, SOL_SOCKET, SO_REUSEADDR,
		   &opt, sizeof(opt)) ;
    if ( n != 0 ) {
	perror("setsockopt SO_REUSEADDR");
    }
    n = setsockopt(fd_socket, SOL_SOCKET, SO_KEEPALIVE,
		   &opt, sizeof(opt)) ;
    if ( n != 0 ) {
	perror("setsockopt SO_KEEPALIVE") ;
    }

    if ( bind(fd_socket, (struct sockaddr *) &socket_addr,
	      sizeof(socket_addr)) < 0 ) {
	perror("bind") ;
	return(-1) ;
    }
    if (listen(fd_socket, BACKLOG) != 0) {
	perror("listen") ;
	return(-1) ;
    }

    return(fd_socket) ;

} /* setup_socket */

void Remote::setup_port(int port, int v)
{
    verbose = v;
    FD_ZERO(&remote_control_connections);
    remote_control_socket = -1;

    remote_control_port = port;
    if (remote_control_port > 0) {

	remote_control_socket = setup_socket("localhost", remote_control_port);
	if (remote_control_socket < 0) {
	    fprintf(stderr, "Cannot setup socket to remote_control_port %d\n",
		    remote_control_port);
	    exit(1) ;
	}
	printf("remote control socket fd is %d\n", remote_control_socket);
	wm->add_fd_to_watch(remote_control_socket);
    }

}


int Remote::get_connection(int socket_fd)
{
    struct sockaddr	socket_addr ;	/* general socket */
    struct sockaddr_in	peer_adr ;	/* inet socket */
    int		fd, n, i ;
    unsigned	char	*pi ;
    socklen_t	len;

    len = sizeof(socket_addr) ;	/* changed by call below */
    if ((fd = accept(socket_fd, &socket_addr, &len)) < 0) {
	perror("accept") ;
	return(-1) ;
    }
    len = sizeof(peer_adr) ;
    n = getpeername(fd, (struct sockaddr *) &peer_adr, &len) ;
    if (n != 0) {
	perror("get_connection: Unable to get peer name") ;
    } else {
	if (verbose) {
	    fprintf(stderr, "peername is ") ;
	}
	if (peer_adr.sin_family != AF_INET) {
	    fprintf(stderr, "Family %d (not AF_INET!) ", peer_adr.sin_family) ;
	} else {
	    if (verbose) {
		fprintf(stderr, " port %d  ip adr ", ntohs(peer_adr.sin_port)) ;
	    }
	    pi = (unsigned char *) &peer_adr.sin_addr ;
	    if (verbose) {
		for (i=0; i<4; i++) {
		    fprintf(stderr, "%d%s", *pi++,  (i==3)?"":".") ;
		}
	    }
	}
	fprintf(stderr, "\n") ;
    }
    return(fd) ;
}




typedef enum {
    err_no_error,
    err_invalid_number,
    err_no_such_window,
    err_missing_args,
    err_invalid_command,
    err_syntax_error,
    err_quit,		// not an error
    err_unknown,	// so we always give a reply
} cmd_errors;

static const char *err_strings[] = {
    "OK\n\n",
    "Invalid number\n\n",
    "No such windowid\n\n",
    "Missing argument(s)\n\n",
    "Invalid command\n\n",
    "Syntax error\n\n",
    "Bye\n\n",
    "Unknown error\n\n",
};

void give_reply(int fd, cmd_errors e) {
    int inv_err = sizeof(err_strings)/sizeof(err_strings[0]);

    if (e >= inv_err) {
	fprintf(stderr, "give_reply: invalid error number %d\n", e);
	e = err_unknown;
    }
    write(fd, err_strings[e], strlen(err_strings[e]));
}
	      
cmd_errors list_clients(WindowManager * mgr, char *buff, int fd) {
    mgr->print_clients(fd);
    return err_no_error;
}


Boolean get_num(char **p, int *num) {
    char *q;
    long x;

    while ( isspace(**p) ) {
	(*p)++;
    }
    x = strtol(*p, &q, 0);
    if ( *q == '\0' || isspace(*q) ) {
	*num = (int) x;
	*p = q;
	return true;
    } else {
	return false;
    }
}

Boolean get_signed_num(char **p, int *num, char *initial) {
    char *q;
    long x;

    while ( isspace(**p) ) {
	(*p)++;
    }
    if (**p == '+' || **p == '-') {
	*initial = **p;
	(*p)++;
    } else {
	*initial = ' ';
    }
    x = strtol(*p, &q, 0);
    if ( *q == '\0' || isspace(*q) ) {
	*num = (int) x;
	*p = q;
	return true;
    } else {
	return false;
    }
}
    

cmd_errors open_client(WindowManager * mgr, char *buff, int fd) {
    char *p;
    int x;
    Window w;

    if (!get_num(&(p=buff), &x)) {
	return err_invalid_number;
    } else {
	w = (Window) x;
	Client *c = mgr->windowToClient(w, false);
	if (c == NULL) {
	    return err_no_such_window;
	} else {
	    c->unhide(True);
	    c->mapRaised();
	    c->ensureVisible();
	}
    }
    return err_no_error;
}

cmd_errors hide_client(WindowManager * mgr, char *buff, int fd) {
    char *p;
    int x;
    Window w;

    if (!get_num(&(p=buff), &x)) {
	return err_invalid_number;
    }
    w = (Window) x;
    Client *c = mgr->windowToClient(w, false);
    if (c == NULL) {
	return err_no_such_window;
    }
    c->hide();
    return err_no_error;
}


cmd_errors unhide_client(WindowManager * mgr, char *buff, int fd) {
    char *p;
    int x;
    Window w;

    if (!get_num(&(p=buff), &x)) {
	return err_invalid_number;
    }
    w = (Window) x;
    Client *c = mgr->windowToClient(w, false);
    if (c == NULL) {
	return err_no_such_window;
    }
    c->unhide(True);
    return err_no_error;
}

cmd_errors move_client(WindowManager * mgr, char *buff, int fd) {
    char *p;
    int x, y;
    Window w;
    char sig_x, sig_y;

    if (!get_num(&(p=buff), &x)) {
	return err_invalid_number;
    }
    w = (Window) x;
    Client *c = mgr->windowToClient(w, false);
    if (c == NULL) {
	return err_no_such_window;
    }

    if (get_signed_num(&p, &x, &sig_x) && 
	get_signed_num(&p, &y, &sig_y)) {
	if (sig_x == '-') {
	    x = DisplayWidth(c->display(), 0) - x;
	}
	if (sig_y == '-') {
	    y = DisplayHeight(c->display(), 0) - y;
	}
	// printf("Moving window 0x%x to (%d, %d)\n", w, x, y);
	c->move(x, y, False);
	// c->ensureVisible();
    } else {
	return err_missing_args;
    }
    return err_no_error;
}

cmd_errors rmove_client(WindowManager * mgr, char *buff, int fd) {
    char *p;
    int x, y;
    Window w;

    if (!get_num(&(p=buff), &x)) {
	return err_invalid_number;
    }
    w = (Window) x;
    Client *c = mgr->windowToClient(w, false);
    if (c == NULL) {
	return err_no_such_window;
    }

    if (get_num(&p, &x) && get_num(&p, &y)) {
	printf("Moving window 0x%x to (%d, %d)\n", w, x, y);
	c->move(x, y, True);
	// c->ensureVisible();
    } else {
	return err_missing_args;
    }
    return err_no_error;
}

cmd_errors maximize_client(WindowManager * mgr, char *buff, int fd) {
    char *p;
    int n;
    Window w;

    if (!get_num(&(p=buff), &n)) {
	return err_invalid_number;
    }
    w = (Window) n;
    Client *c = mgr->windowToClient(w, false);
    if (c == NULL) {
	return err_no_such_window;
    }

    if (get_num(&p, &n)){
	printf("maximize_client window 0x%x n =  %d\n", w, n);
	c->maximise(n);
    } else {
	return err_missing_args;
    }
    return err_no_error;
}

cmd_errors unmaximize_client(WindowManager * mgr, char *buff, int fd) {
    char *p;
    int n;
    Window w;

    if (!get_num(&(p=buff), &n)) {
	return err_invalid_number;
    }
    w = (Window) n;
    Client *c = mgr->windowToClient(w, false);
    if (c == NULL) {
	return err_no_such_window;
    }

    if (get_num(&p, &n)){
	printf("unmaximize_client window 0x%x n =  %d\n", w, n);
	c->unmaximise(n);
    } else {
	return err_missing_args;
    }
    return err_no_error;
}

cmd_errors raise_cmd(WindowManager * mgr, char *buff, int fd) {
    char *p;
    int x;
    Window w;

    if (!get_num(&(p=buff), &x)) {
	return err_invalid_number;
    }
    w = (Window) x;
    Client *c = mgr->windowToClient(w, false);
    if (c == NULL) {
	return err_no_such_window;
    }
    c->mapRaised();
    return err_no_error;
}

cmd_errors raise_or_lower_cmd(WindowManager * mgr, char *buff, int fd) {
    char *p;
    int x;
    Window w;

    if (!get_num(&(p=buff), &x)) {
	return err_invalid_number;
    }
    w = (Window) x;
    Client *c = mgr->windowToClient(w, false);
    if (c == NULL) {
	return err_no_such_window;
    }
    c->raiseOrLower();
    return err_no_error;
}

cmd_errors lower_cmd(WindowManager * mgr, char *buff, int fd) {
    char *p;
    int x;
    Window w;

    if (!get_num(&(p=buff), &x)) {
	return err_invalid_number;
    }
    w = (Window) x;
    Client *c = mgr->windowToClient(w, false);
    if (c == NULL) {
	return err_no_such_window;
    }
    c->lower();
    return err_no_error;
}

cmd_errors relabel_cmd(WindowManager * mgr, char *buff, int fd) {
    char *p;
    int x;
    Window w;

    if (!get_num(&(p=buff), &x)) {
	return err_invalid_number;
    }
    w = (Window) x;
    Client *c = mgr->windowToClient(w, false);
    if (c == NULL) {
	return err_no_such_window;
    }
    while (isspace(*p)) {
	p++;
    }
    if (*p == '\0') {
	return err_missing_args;
    }
    c->setLabel(p);
    return err_no_error;
}

cmd_errors warp_cmd(WindowManager * mgr, char *buff, int fd) {
    char *p;
    int x;
    Window w;

    if (!get_num(&(p=buff), &x)) {
	return err_invalid_number;
    }
    w = (Window) x;
    Client *c = mgr->windowToClient(w, false);
    if (c == NULL) {
	return err_no_such_window;
    }

    c->unhide(True);
    c->activateAndWarp();

    return err_no_error;
}

cmd_errors window_to_channel_cmd(WindowManager * mgr, char *buff, int fd) {
    char *p;
    int x;
    Window w;
    Boolean leaving;
    int active_channel = mgr->channel();
    int channel;

    if (!get_num(&(p=buff), &x)) {
	return err_invalid_number;
    }
    w = (Window) x;
    Client *c = mgr->windowToClient(w, false);
    if (c == NULL) {
	return err_no_such_window;
    }
    if (!get_num(&p, &channel)) {
	return err_invalid_number;
    }

    leaving = (channel != active_channel);
    if (!leaving) {
	c->setChannel(channel);
    }
    c->flipChannel(leaving, channel);
    if (leaving) {
	c->setChannel(channel);
    }
    return err_no_error;;
}


cmd_errors go_to_channel_cmd(WindowManager * mgr, char *buff, int fd) {
    char *p;
    int x;
    char temp[1024];

    if (strlen(buff) == 0) {
	// no channel specified,  show current one
	snprintf(temp, 1024, "Current channel is %d\n", mgr->channel());
	write(fd, temp, strlen(temp));
	return err_no_error;
    }
    if (!get_num(&(p=buff), &x)) {
	return err_invalid_number;
    }
    mgr->gotoChannel(x, 0);
    return err_no_error;
}

cmd_errors quit_cmd(WindowManager * mgr, char *buff, int fd) {
    return err_quit;
}

cmd_errors options_cmd(WindowManager * mgr, char *buff, int fd) {
    char *p;

    fprintf(stderr, "processing options %s\n", buff);
    p = buff;
    while (isspace(*p)) p++;
    if (*p) {
	if (DynamicConfig::config.update(p) == 0) {
	    return err_no_error;
	}
    } 
    return err_syntax_error;
}

// typedef int (WindowManager::*command_func)(char *buff, int fd);
// typedef int (*command_func)(WindowManager * mgr, char *buff, int fd);
typedef cmd_errors(*command_func)(WindowManager * mgr, char *buff, int fd);
// typedef cmd_errors(*command_func)(Remote * mgr, char *buff, int fd);

int Remote::parse_command(char *buff, int fd)
{
    int i;
    char *p;
    char reply[128];
    struct {
	const char *command;
	const char *usage;
	command_func func;
    } table[] = {
	{ "channel", " [<number>]", go_to_channel_cmd },
	{ "clients", "", &list_clients },
	{ "help", "", NULL },
	{ "hide", " <window id>", hide_client },
	{ "label", " <window id> <any string>", relabel_cmd },
	{ "lower", " <window id>", lower_cmd },
	{ "maximize", " <window id> <number>", maximize_client },
	{ "move", " <window id> <x> <y>", move_client },
	{ "open", " <window id>", open_client },
	{ "options", " <options string>", options_cmd },
	{ "quit", "", quit_cmd },
	{ "raise", " <window id>", raise_cmd },
	{ "raiseorlower", " <window id>", raise_or_lower_cmd },
	{ "rmove", " <window id> <x> <y>", rmove_client },
	{ "unhide", " <window id>", unhide_client },
	{ "unmaximize", " <window id> <number>", unmaximize_client },
	{ "warp", " <window id>", warp_cmd },
	{ "wchannel", " <window id> <number>", window_to_channel_cmd },
    };
    int num_entries = sizeof(table)/sizeof(table[0]);
    command_func func;
    cmd_errors err;

    // Remove any trailing newline/cr/lf
    p = buff + strlen(buff) - 1;
    while (*p == '\n' || *p == '\r') {
	*p = '\0';
	p--;
    }
    // Find where the first arg (if any) is
    p = buff;
    while (*p && !isspace(*p)) {
	p++;
    }

    err = err_invalid_command;
    for (i=0; i< num_entries; i++) {
	if (strncmp(table[i].command, buff, strlen(table[i].command)) == 0) {
	    func = table[i].func;
	    if (func == NULL) {
		// do help here since the table is local
		int j;
		snprintf(reply, 20, "Commands are:\n");
		write(fd, reply, strlen(reply));
		for (j=0; j< num_entries; j++) {
		    write(fd, table[j].command, strlen(table[j].command));
		    write(fd, table[j].usage, strlen(table[j].usage));
		    write(fd, "\n", 1);
		}
		err = err_no_error;
		break;
	    }
	    // err = (*func)(this, p, fd);
	    err = (*func)(wm, p, fd);
	    break;
	}
    }
    if (err == err_quit) {
	// closeConnection(fd);
	return -1;
    }
    give_reply(fd, err);
    if (sigpipe_happened) {
	sigpipe_happened = False;
	return -1;
    }
    if (err == err_no_error) {
	return 0;
    } else {
	return 1;
    }
}

Boolean Remote::isRemoteControlFd(int fd)
{
    return (fd == remote_control_socket) ||
	(FD_ISSET(fd, &remote_control_connections));
}

void Remote::closeConnection(int fd)
{
    close(fd);
    FD_CLR(fd, &remote_control_connections);
    wm->remove_fd_from_watch(fd);

}

void Remote::close_all_sockets(void) {
    int fd;

    if (remote_control_socket > 0) {
	if (verbose) {
	    printf("closing %d\n", remote_control_socket);
	}
	close(remote_control_socket);
    }
    for (fd = 3; fd < max_connection; fd++) {
	if (FD_ISSET(fd, &remote_control_connections)) {
	    printf("closing %d\n", fd);
	    close(fd);
	}
    }
}


Boolean Remote::doRemoteControl(int fd)
{
    int n;
    int new_fd;
    char welcome_msg[] = 
	"Welcome to wmx remote control.  Type help for commands\n\n";

    if (verbose) {
	printf("WindowManager::doRemoteControl(%d)\n", fd);
    }

    if (fd == remote_control_socket) {
	/* A new connection */
	new_fd = get_connection(remote_control_socket);
	if (new_fd < 0) {
	    fprintf(stderr, "Error trying to get new connection\n");
	    return False;
	}
	if (verbose) {
	    printf("New connection on %d:  %d\n", 
		   fd, new_fd);
	}
	if (new_fd >= max_connection) {
	    int size;
	    max_connection = new_fd + 1;
	    // printf("max_connection is now %d\n", max_connection);
	    size = max_connection * sizeof(Connection*);
	    // printf("reallocating to size %d\n", size);
	    connections = (Connection **)
		realloc(connections, size);
	}
	// printf("setting connections[%d]\n", new_fd);
	connections[new_fd] = new Connection(1024, new_fd);

	FD_SET(new_fd, &remote_control_connections);
	wm->add_fd_to_watch(new_fd);
	write(new_fd, welcome_msg, strlen(welcome_msg));
	return True;
    }


    if (FD_ISSET(fd, &remote_control_connections)) {
	/* Activity on existing connection */
	Connection *c = connections[fd];
	n = c->store_input(this);
	if (n < 0) {
	    if (verbose) {
		printf("Closing connection %d\n", fd);
	    }
	    delete c;
	    connections[fd] = NULL;
	    closeConnection(fd);
	    return False;
	}
	return True;

    }
    fprintf(stderr,
	    "WindowManager::doRemoteControl(%d) -- why were we called?\n", fd);
    return False;
}


