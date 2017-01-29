#define DEBUG 1
/* debug functions identically to printf except that it only prints if 
 * the DEBUG variable is set and it always prints to stderr
 */
void debug(const char* format, ...);
