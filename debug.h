#ifndef DEBUG_H
#define DEBUG_H

#define DEBUG 0
/* debug functions identically to printf except that it only prints if 
 * the DEBUG variable is set and it always prints to stderr
 */
void debug(const char* format, ...);

#endif
