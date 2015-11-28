#ifndef ALOS_KPRINT_H
#define ALOS_KPRINT_H

//! Use this macro to mark errors
#define KPRINT_ERR "[ERROR]   "

//! Use this macro to mark warnings
#define KPRINT_WARN "[WARNING] "

//! Use this macro to mark information messages
#define KPRINT_MSG "[MESSAGE] "

//! Use this macro to mark debug traces
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
