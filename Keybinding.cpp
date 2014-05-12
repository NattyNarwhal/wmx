#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include <sys/wait.h>

#include "Keybinding.h"

int Keybinding::scan_directory(char *dir_name, bool with_modifier)
{

    DIR *dir;
    struct dirent *ent;
    struct stat buf;
    int n;
    char filename[1024];
    int pass = 0;
    int count = 0;
    struct key_entry *p;
    KeySym sym;

    table_count = 0;
    n = stat(dir_name, &buf);
    if (n != 0 || !S_ISDIR(buf.st_mode)) {
	printf("Directory %s does not exist\n", dir_name);
	return -1;
    }
    dir = opendir(dir_name);
    if (dir == NULL) {
	printf("Could not open directory %s\n", dir_name);
	return -1;
    }

    for (pass=0; pass<2; pass++) {
	count = 0;
	while ((ent = readdir(dir))) {
	    // printf("%s type: %d\n", ent->d_name, ent->d_type);
	    sprintf(filename, "%s/%s", dir_name, ent->d_name);
	    n = stat(filename, &buf);
	    // printf("type 0%o\n", buf.st_mode);
	    if (n != 0) {
		printf("stat failed for %s, n = %d\n", filename, n);
		continue;
	    }
	    if (!S_ISREG(buf.st_mode)) {
		// printf("not a regular file\n");
		continue;
	    }
	    if (S_ISDIR(buf.st_mode)) {
		// printf("is a directory\n");
		continue;
	    }
	    // S_ISLNK(buf.st_mode) doesn't work
	    // but we probably don't need to exclude them anyway

	    if (! (buf.st_mode & S_IXUSR) ) {
		// printf("is NOT executable\n");
		continue;
	    }
	    sym = XStringToKeysym(ent->d_name);
	    if (sym == 0) {
		printf("not suitable filename: %s\n", ent->d_name);
		continue;
	    }

	    // it is valid
	    // printf("valid filename %s  sym is 0x%x\n", filename, sym);

	    count++;
	    if (pass == 0) {
		continue;
	    } else {
		if (count > table_count) {
		    printf("internal error count=%d table_count=%d\n",
			   count, table_count);
		    exit(2);
		}
		p->sym = sym;
		p->command = strdup(filename);
		p++;
	    }

	} // while readdir
	if (pass == 0) {
	    // end of first pass
	    // printf("There were %d entries found\n", count);
	    table_count = count;
	    p = table = (struct key_entry *)
		malloc( count * sizeof(struct key_entry) );
	    if (table == NULL) {
		printf("Cannot allocate user key table\n");
		exit(1);
	    }
	    rewinddir(dir);
	}
    }
    if (with_modifier) {
      printf("These keys are dynamically bound if the modifier key is active\n");
    } else {
      printf("These keys are dynamically bound if there is no modifier.\n");
    }

    int i;
    for (i=0; i < table_count; i++) {
	printf("%s : %s\n", XKeysymToString(table[i].sym), table[i].command);
    }

    closedir(dir);

}

Keybinding::Keybinding(WindowManager *const w_mgr, char *dir, 
		       bool with_modifier)
{
    char key_dir_name[1024];
    int n;
    char *home = getenv("HOME");
    const char *keys_name = with_modifier ? ".mkeys" : ".keys";

    wm = w_mgr;
    if (dir) {
	snprintf(key_dir_name, 1024, "%s/%s", dir, keys_name);
    } else {
	snprintf(key_dir_name, 1024, "%s/%s/%s", home, CONFIG_COMMAND_MENU, keys_name);
    }
    n = scan_directory(key_dir_name, with_modifier);

}

Keybinding::~Keybinding()
{}


void Keybinding::setup(void) 
{}

char *Keybinding::command_for_keysym(KeySym sym)
{
    int i;

    for (i=0; i < table_count; i++) {
	if (sym == table[i].sym) {
	    return table[i].command;
	}
    }
    return NULL;
}


void Keybinding::spawn(char *command, int windowid)
{
    char client_id[1024];
    snprintf(client_id, 1024, "0x%x", windowid);

    // printf("arg1 is %s\n", client_id);
    wm->spawn(command, command, client_id);

}

void Keybinding::grab_each_key(Display *display, Window window, int mask) {
    int i;
    int keycode;

    for (i=0; i < table_count; i++) {
	keycode = XKeysymToKeycode(display, table[i].sym);
	XGrabKey(display, keycode,
		 mask,
		 window, False,
		 GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycode,
		 mask|LockMask|Mod2Mask,
		 window, False,
		 GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycode,
		 mask|LockMask,
		 window, False,
		 GrabModeAsync, GrabModeAsync);
	XGrabKey(display, keycode,
		 mask|Mod2Mask,
		 window, False,
		 GrabModeAsync, GrabModeAsync);
    }
}
