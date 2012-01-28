
#ifdef DEBUG
#undef DEBUG
#endif

#define Y_UNIX
#define Y_X11
#define Y_GETTIMEOFDAY
#define Y_NANOSLEEP
#define Y_SNPRINTF
#define Y_USLEEP

extern const char* yadex_etc_path[];
extern const char* yadex_share_path[];
