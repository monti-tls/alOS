#ifndef ALOS_KPRINT_H
#define ALOS_KPRINT_H

#define KPRINT_ERR "[ERROR]   "
#define KPRINT_WARN "[WARNING] "
#define KPRINT_MSG "[MESSAGE] "
#define KPRINT_TRACE "[TRACE]   "

//! Init the kprint module (sets up
//!   SWO and ITP things)
//! \return 0 if successful, -1 otherwise
int kprint_init();

//! Print some debug information to the kernel's
//!   debug port (printf-like).
//! \param format The printf-like format string
void __attribute__((format(printf, 1, 2))) kprint(const char* fmt, ...);

#endif // ALOS_KPRINT_H
