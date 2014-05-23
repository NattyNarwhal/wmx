#include <string.h>

#ifdef linux
extern size_t strlcat(char *, const char *, size_t);
extern size_t strlcpy(char *, const char *, size_t);
#endif
