/* lwip necessary configurations, add any necessary new
config here
*/

/* set up the rand function as the system function rand */
#define LWIP_RAND() rand()

/* enable IGMP */
#define LWIP_IGMP 1

/* include the default lwipopts.h */
#include_next <lwipopts.h> 
