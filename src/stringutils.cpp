#include "dbat/stringutils.h"

#ifdef _WIN32
#include "dbat/utils.h"
// Case-insensitive comparison of two C-strings (similar to strcasecmp)
int strcasecmp(const char* s1, const char* s2) {
    std::string str1(s1);
    std::string str2(s2);
    if (iequals(str1, str2)) return 0;
    // For a more accurate mimic of `strcasecmp`, you might need to compare the strings
    // in a way that reflects their lexicographical order after conversion to a common case.
    return str1 < str2 ? -1 : 1;
}

// Case-insensitive comparison of two C-strings with a length limit (similar to strncasecmp)
int strncasecmp(const char* s1, const char* s2, size_t n) {
    std::string str1(s1, n);
    std::string str2(s2, n);
    if (iequals(str1, str2)) return 0;
    // Adjust comparison logic as needed for correct lexicographical order.
    return str1 < str2 ? -1 : 1;
}
#endif

size_t strlcpy(char *dst, const char *src, size_t dsize)
{
    const char *osrc = src;
    size_t nleft = dsize;

    /* Copy as many bytes as will fit. */
    if (nleft != 0) {
        while (--nleft != 0) {
            if ((*dst++ = *src++) == '\0')
                break;
        }
    }

    /* Not enough room in dst, add NUL and traverse rest of src. */
    if (nleft == 0) {
        if (dsize != 0)
            *dst = '\0';		/* NUL-terminate dst */
        while (*src++)
            ;
    }

    return(src - osrc - 1);	/* count does not include NUL */
}