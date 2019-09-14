// ============================================================================
//
// = LIBRARY
//    ULib - c++ library
//
// = FILENAME
//    timer.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_TIMER_H
#define ULIB_TIMER_H

#include <ulib/event/event_time.h>
#include <ulib/utility/interrupt.h>

class UNotifier;
class UServer_Base;

// UNotifier use this class to notify a timeout from select()

class U_EXPORT UTimer {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   enum Type { SYNC, ASYNC, NOSIGNAL };

   UTimer()
      {
      U_TRACE_CTOR(0, UTimer, "")

      next  = U_NULLPTR;
      alarm = U_NULLPTR;
      }

   ~UTimer()
      {
      U_TRACE_DTOR(0, UTimer)
      }

   // SERVICES

   static bool empty()
      {
      U_TRACE_NO_PARAM(0, "UTimer::empty()")

      if (first == U_NULLPTR) U_RETURN(true);

      U_RETURN(false);
      }

   static bool isAlarm()
      {
      U_TRACE_NO_PARAM(0, "UTimer::isAlarm()")

         U_INTERNAL_DUMP("UInterrupt::timerval.it_value = { %ld %6ld } UInterrupt::timerval.it_interval = { %ld %6ld }", UInterrupt::timerval.it_value.tv_sec,
                          UInterrupt::timerval.it_value.tv_usec,       UInterrupt::timerval.it_interval.tv_sec,          UInterrupt::timerval.it_interval.tv_usec)

      if (UInterrupt::timerval.it_value.tv_sec  != 0 ||
          UInterrupt::timerval.it_value.tv_usec != 0)
         {
         U_RETURN(true);
         }

      U_RETURN(false);
      }

   static void init(Type _mode) // initialize the timer
      {
      U_TRACE(0, "UTimer::init(%d)", _mode)

      if ((mode = _mode) != NOSIGNAL)
         {
              if (_mode ==  SYNC) UInterrupt::setHandlerForSignal(SIGALRM, (sighandler_t)UTimer::handlerAlarm);
         else if (_mode == ASYNC) UInterrupt::insert(             SIGALRM, (sighandler_t)UTimer::handlerAlarm); // async signal
         }
      }

   static void clear();                    // cancel all timers and free storage, usually in preparation for exitting
   static void insert(UEventTime* palarm); // set up a timer, either periodic or one-shot

   // deschedule a timer. Note that non-periodic timers are automatically descheduled when they run, so you don't have to call this on them

   static void erase(UTimer* item)
      {
      U_TRACE(0, "UTimer::erase(%p)", item)

      U_INTERNAL_ASSERT_POINTER(first)

      if (mode != NOSIGNAL) U_DELETE(item)
      else
         {
         // put it on the free list

         item->next = pool;
               pool = item;
         }
      }

   static void erase(UEventTime* palarm);

   // run the list of timers. Your main program needs to call this every so often, or as indicated by getTimeout()

   static void run();
   static void setTimer();

   static UEventTime* getTimeout() // returns a timeout indicating how long until the next timer triggers
      {
      U_TRACE_NO_PARAM(0, "UTimer::getTimeout()")

      if (        first &&
          (run(), first))
         {
         UEventTime* a = first->alarm;

         U_ASSERT(a->checkTolerance())

         U_RETURN_POINTER(a, UEventTime);
         }

      U_RETURN_POINTER(U_NULLPTR, UEventTime);
      }

   static bool isHandler(UEventTime* palarm)
      {
      U_TRACE(0, "UTimer::isHandler(%p)", palarm)

      for (UTimer* item = first; item; item = item->next)
         {
         if (item->alarm == palarm)
            {
            U_INTERNAL_DUMP("item = %p", item)

            U_RETURN(true);
            }
         }

      U_RETURN(false);
      }

   // manage signal

   static RETSIGTYPE handlerAlarm(int signo)
      {
      U_TRACE(0, "[SIGALRM] UTimer::handlerAlarm(%d)", signo)

      setTimer();
      }

   // STREAM

#ifdef U_STDCPP_ENABLE
   friend U_EXPORT ostream& operator<<(ostream& os, const UTimer& t);

   // DEBUG

# ifdef DEBUG
   static void   printInfo(ostream& os);
          void outputEntry(ostream& os) const U_NO_EXPORT;

   const char* dump(bool reset) const;
# endif
#endif

protected:
   UTimer* next;
   UEventTime* alarm;

   static int mode;
   static UTimer* pool;  //   free list 
   static UTimer* first; // active list 

   static void callHandlerTimeout();
   static void updateTimeToExpire(UEventTime* ptime);

#ifdef DEBUG
   static bool invariant();
#endif

private:
   void insertEntry() U_NO_EXPORT;

   bool operator< (const UTimer& t) const { return (*alarm < *t.alarm); }
   bool operator> (const UTimer& t) const { return  t.operator<(*this); }
   bool operator<=(const UTimer& t) const { return !t.operator<(*this); }
   bool operator>=(const UTimer& t) const { return !  operator<(t); }
   bool operator==(const UTimer& t) const { return (*alarm == *t.alarm); }
   bool operator!=(const UTimer& t) const { return !  operator==(t); }

   U_DISALLOW_COPY_AND_ASSIGN(UTimer)

   friend class UNotifier;
   friend class UServer_Base;
};

#endif
