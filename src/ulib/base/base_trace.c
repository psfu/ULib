/* ============================================================================
//
// = LIBRARY
//    ULib - c library
//
// = FILENAME
//    base_trace.c
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================ */
 
/*
#define DEBUG_DEBUG
*/

#include <ulib/base/trace.h>
#include <ulib/base/utility.h>

int      u_trace_fd = -1;
int      u_trace_suspend;
bool     u_trace_signal;
char     u_trace_tab[256]; /* 256 max indent */
void*    u_trace_mask_level;
uint32_t u_trace_num_tab;

static int level_active;
static uint32_t file_size;
#ifdef ENABLE_THREAD
static uint32_t old_tid;
# ifdef _MSWINDOWS_
static CRITICAL_SECTION mutex;
# else
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
# endif
#endif

/*
#ifdef __UNIKERNEL__
#  include <rump/rump.h>
#  include <rump/rump_syscalls.h>

#  define  open(...) rump_sys_open(__VA_ARGS__)
#  define write(...) rump_sys_write(__VA_ARGS__)
#  define close(...) rump_sys_close(__VA_ARGS__)
#endif
*/

static char* restrict file_mem;
static char* restrict file_ptr;
static char* restrict file_limit;

void u_trace_check_if_interrupt(void) /* check for context manage signal event - interrupt */
{
   U_INTERNAL_TRACE("u_trace_check_if_interrupt()")

   if (file_size            &&
       file_ptr != file_mem &&
       *(file_ptr-1) != '\n')
      {
      *file_ptr++ = '\n';
      }
}

void u_trace_lock(void)
{
#ifdef ENABLE_THREAD
   uint32_t tid = u_gettid();

# ifdef _MSWINDOWS_
   if (old_tid == 0) InitializeCriticalSection(&mutex);

   EnterCriticalSection(&mutex);
# else
   (void) pthread_mutex_lock(&mutex);
# endif

   U_INTERNAL_TRACE("u_trace_lock()")

   if (old_tid != tid)
      {
      char tid_buffer[32];
      int sz = snprintf(tid_buffer, sizeof(tid_buffer), "[tid %u]<--\n[tid %u]-->\n", old_tid, tid);

      old_tid = tid;

      if (file_size == 0) (void) write(u_trace_fd, tid_buffer, sz);
      else
         {
         if ((file_ptr + sz) > file_limit) file_ptr = file_mem;

         u__memcpy(file_ptr, tid_buffer, sz, __PRETTY_FUNCTION__);

         file_ptr += sz;
         }
      }
#endif
}

void u_trace_unlock(void)
{
   U_INTERNAL_TRACE("u_trace_unlock()")

#ifdef ENABLE_THREAD
# ifdef _MSWINDOWS_
   LeaveCriticalSection(&mutex);
# else
   (void) pthread_mutex_unlock(&mutex);
# endif
#endif
}

void u_trace_writev(const struct iovec* restrict iov, int n)
{
   U_INTERNAL_TRACE("u_trace_writev(%p,%d)", iov, n)

   U_INTERNAL_ASSERT_MINOR(u_trace_num_tab, sizeof(u_trace_tab))

   if (file_size == 0)
      {
      if (u_trace_fd != -1) (void) writev(u_trace_fd, iov, n);
      }
   else
      {
      int i;

      u_trace_lock();

      for (i = 0; i < n; ++i)
         {
      /* U_INTERNAL_PRINT("iov[%d].iov_len = %d iov[%d].iov_base = %p", i, iov[i].iov_len, i, iov[i].iov_base) */

         if (iov[i].iov_len > 0)
            {
            U_INTERNAL_ASSERT_POINTER(iov[i].iov_base)

            if ((file_ptr + iov[i].iov_len) > file_limit) file_ptr = file_mem;

            u__memcpy(file_ptr, iov[i].iov_base, iov[i].iov_len, __PRETTY_FUNCTION__);

            file_ptr += iov[i].iov_len;
            }
         }

      u_trace_unlock();
      }
}

void u_trace_write(const char* restrict t, uint32_t tlen)
{
   struct iovec iov[3] = { { (caddr_t)u_trace_tab, u_trace_num_tab },
                           { (caddr_t)t, tlen },
                           { (caddr_t)"\n", 1 } };

   U_INTERNAL_TRACE("u_trace_write(%s,%u)", t, tlen)

   u_trace_writev(iov, 3);
}

void u_trace_close(void)
{
   /* NB: we disable trace if we are on signal handler in call system... */

   int lfd = u_trace_fd;
             u_trace_fd = -1;

   U_INTERNAL_TRACE("u_trace_close()")

   if (lfd != -1 &&
       lfd > STDERR_FILENO)
      {
      if (file_size)
         {
         ptrdiff_t write_size = file_ptr - file_mem;

         U_INTERNAL_ASSERT_MINOR(write_size, (ptrdiff_t)file_size)

      // (void)  msync(file_mem, write_size, MS_SYNC | MS_INVALIDATE);
         (void) munmap(file_mem,  file_size);

         if (write_size > 0)
            {
            (void) ftruncate(lfd, write_size);
         // (void) fsync(lfd);
            }

         file_size = 0;
         }

      (void) close(lfd);
      }
}

static int flag_init;
static struct sigaction act;

static RETSIGTYPE handlerSIGUSR2(int signo)
{
   U_VAR_UNUSED(signo)

   U_INTERNAL_TRACE("handlerSIGUSR2(%d)", signo)

   u_fork_called  = false;
   u_trace_signal = true;

/*
#ifndef _MSWINDOWS_
   setHandlerSIGUSR2(); // on-off by signal SIGUSR2
#endif
*/
}

static void setHandlerSIGUSR2(void)
{
   U_INTERNAL_TRACE("setHandlerSIGUSR2()")

   act.sa_handler = handlerSIGUSR2;

   (void) sigaction(SIGUSR2, &act, 0);
}

void u_trace_init(bool bsignal)
{
   const char* env;

   U_INTERNAL_TRACE("u_trace_init(%d)", bsignal)

   flag_init = true;

   env = getenv(bsignal ? "UTRACE_SIGNAL" : "UTRACE");

   U_INTERNAL_ASSERT_POINTER(u_trace_folder)

   if (bsignal == false     &&
       ( env ==   U_NULLPTR ||
        *env == '\0'))
      {
      level_active = -1;
      }
   else
      {
      char suffix;

      if (env == 0) env = "0 50M 0";
      else
         {
         if (u__isquote(*env)) ++env; /* normalization... */

         if (*env == '-')
            {
            ++env;

            u_trace_fd = STDERR_FILENO;
            }
         }

      /**
       * format: <level> <max_size_log> <u_flag_test>
       *            -1        500k           0
       */

      (void) sscanf(env, "%d%d%c%d", &level_active, &file_size, &suffix, &u_flag_test);

      if (file_size) U_NUMBER_SUFFIX(file_size, suffix);
      }

   (void) memset(u_trace_tab, '\t', sizeof(u_trace_tab));

   if (level_active >= 0)
      {
      u_trace_mask_level = 0; /* NB: reset necessary for incoherent state... */

      if (u_trace_fd == STDERR_FILENO) file_size = 0;
      else
         {
         char name[U_PATH_MAX+1];

         U_INTERNAL_ASSERT_POINTER(u_trace_folder)

         (void) u__snprintf(name, U_PATH_MAX, U_CONSTANT_TO_PARAM("%s/trace.%N.%P"), u_trace_folder);

         /* NB: O_RDWR is needed for mmap(MAP_SHARED)... */

         u_trace_fd = open(name, O_CREAT | O_TRUNC | O_RDWR | O_BINARY | O_APPEND, 0666);

         if (u_trace_fd == -1)
            {
            U_WARNING("Failed to create file %S%R - current working directory: %.*S - UTRACE_FOLDER: %S", name, 0, u_cwd_len, u_cwd, u_trace_folder);

            file_size = 0;

            return;
            }

         /* we manage max size... */

         if (file_size)
            {
            off_t start = (u_fork_called ? lseek(u_trace_fd, 0, SEEK_END) : 0);

            if (ftruncate(u_trace_fd, file_size))
               {
               U_WARNING("Out of space on file system, (required %u bytes)", file_size);

               file_size = 0;

               return;
               }

            /* NB: to include also PROT_READ seem to avoid some strange SIGSEGV... */

            file_mem = (char* restrict) mmap(0, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, u_trace_fd, 0);

            if (file_mem == MAP_FAILED)
               {
               file_mem  = 0;
               file_size = 0;

               (void) ftruncate(u_trace_fd, 0);
               }

            file_ptr   = file_mem + start;
            file_limit = file_mem + file_size;
            }
         }

      if (u_trace_fd > STDERR_FILENO) u_atexit(&u_trace_close); /* register function of close trace at exit... */
      }

#ifndef _MSWINDOWS_
   setHandlerSIGUSR2(); /* on-off by signal SIGUSR2 */
#endif
}

void u_trace_handlerSignal(void)
{
   U_INTERNAL_TRACE("u_trace_handlerSignal()")

   u_trace_signal = false;

#ifdef DEBUG
   if (u_trace_fd == -1 ||
       level_active < 0)
      {
      u_trace_init(true);

      u_trace_unlock();
      }
   else
      {
      u_trace_close();

      level_active = -1;
      }
#endif
}

void u_print_status_trace(void)
{
   U_INTERNAL_TRACE("u_print_status_trace()")

   if (u_trace_fd == -1)
      {
      U_MESSAGE("TRACE%W<%Woff%W>%W", YELLOW, RED, YELLOW, RESET);
      }
   else
      {
      U_MESSAGE("TRACE%W<%Won%W>: Level<%W%d%W> MaxSize<%W%d%W> Test<%W%d%W>%W", YELLOW,
                  GREEN,              YELLOW,
                  CYAN, level_active, YELLOW,
                  CYAN, file_size,    YELLOW,
                  CYAN, u_flag_test,  YELLOW,
                  RESET);
      }
}

/* we can mask trace by hi-byte param 'level' */

int u_trace_check_if_active(int level)
{
   U_INTERNAL_TRACE("u_trace_check_if_active(%d)", level)

   int trace_active;

        if (flag_init == 0) u_trace_init(false);
   else if (u_trace_signal)
      {
      u_trace_handlerSignal();

      u_print_status_trace();

      if (u_trace_fd == -1) return 0;
      }

   U_INTERNAL_PRINT("u_trace_fd = %d level_active = %d u_trace_mask_level = %p", u_trace_fd, level_active, u_trace_mask_level)

        if (u_trace_fd == -1 || u_trace_suspend) trace_active = 0;
   else if (     level == -1)                    trace_active = (level_active == 0);
   else                                          trace_active = (u_trace_mask_level == 0 && ((level & 0x000000ff) >= level_active));

   U_INTERNAL_PRINT("trace_active = %d u_flag_test = %d", trace_active, u_flag_test)

   return (u_flag_test >= 0 ? trace_active : 1);
}

void u_trace_check_init(void)
{
   U_INTERNAL_TRACE("u_trace_check_init()")

   /* we check if there are previous creation of global objects that can have forced the initialization of trace file... */

   if (flag_init         &&
       level_active >= 0 &&
       u_trace_fd != STDERR_FILENO)
      {
      char name[128];

      (void) u__snprintf(name, 128, U_CONSTANT_TO_PARAM("trace.%N.%P"), 0);

      (void) rename("trace..", name);
      }
   else
      {
      u_trace_init(false);
      }
}

void u_trace_dump(const char* restrict format, uint32_t fmt_size, ...)
{
   char buffer[8192];
   uint32_t buffer_len;

   va_list argp;
   va_start(argp, fmt_size);

   U_INTERNAL_TRACE("u_trace_dump(%.*s,%u)", fmt_size, format, fmt_size)

   buffer_len = u__vsnprintf(buffer, U_CONSTANT_SIZE(buffer), format, fmt_size, argp);

   va_end(argp);

   if (buffer_len)
      {
      struct iovec iov[3] = { { (caddr_t)u_trace_tab, u_trace_num_tab },
                              { (caddr_t)buffer, buffer_len },
                              { (caddr_t)"\n", 1 } };

      u_trace_writev(iov, 3);
      }
}

void u_trace_initFork(void)
{
   U_INTERNAL_TRACE("u_trace_initFork()")

   bool bsignal = (u_trace_fd != -1);

   if (u_trace_fd > STDERR_FILENO)
      {
      if (file_size)
         {
         (void) munmap(file_mem, file_size);
                                 file_size = 0;
         }

      (void) close(u_trace_fd);
                   u_trace_fd = -1;
      }

   u_trace_init(bsignal);
}

/*
#ifdef __UNIKERNEL__
#  undef  open
#  undef write
#  undef close
#endif
*/
