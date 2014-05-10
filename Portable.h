#include <string.h>

#if !HAVE_STRLCAT
extern size_t strlcat(char *, const char *, size_t);
#endif
#if !HAVE_STRLCPY
extern size_t strlcpy(char *, const char *, size_t);
#endif
