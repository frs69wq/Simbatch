#include "simbatch_config.h"
#include "sb_config.h"


inline void SB_global_init(int * argc, char ** argv) {
    simbatch_init(argc, argv);
}


/* Get the platform filename from the config */
inline const char * SB_get_platform_file(void) {
    return config_get_platform_file();
}


/* Get the platform filename from the config */
inline const char * SB_get_deployment_file(void) {
    return config_get_deployment_file();
}


inline const char * SB_get_trace_file(void) {
    return config_get_trace_file();
}


inline void SB_clean(void) {
    simbatch_clean();
}

