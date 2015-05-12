#ifndef _SPLICE_SCOPE_TIMER
#define _SPLICE_SCOPE_TIMER

#define FABRIC_OS_WINDOWS

#include <stdint.h>
#if defined(FABRIC_OS_LINUX)
# include <time.h>
# include <sys/time.h>
#elif defined(FABRIC_OS_DARWIN)
# include <mach/mach_time.h>
#elif defined(FABRIC_OS_WINDOWS)
# include <windows.h>
#else
# error "Unsupported OS"
#endif

inline uint64_t GetCurrentTicks()
{
#if defined(FABRIC_OS_WINDOWS)
  LARGE_INTEGER counts;
  ::QueryPerformanceCounter( &counts );
  return uint64_t( counts.QuadPart );
#elif defined(FABRIC_OS_LINUX)
  struct timespec tv;
  clock_gettime( CLOCK_MONOTONIC, &tv );
  return uint64_t(tv.tv_sec) * uint64_t(1000000000) + uint64_t(tv.tv_nsec);
#elif defined(FABRIC_OS_DARWIN)
  return mach_absolute_time();
#else
# error "Unsupported OS"
#endif
}

inline double GetSecondsForTicksDiff( uint64_t ticksDiff )
{
#if defined(FABRIC_OS_WINDOWS)
  static double secondsPerTick = 0.0;
  if ( !secondsPerTick )
  {
    LARGE_INTEGER ticksPerSecond;
    ::QueryPerformanceFrequency( &ticksPerSecond );
    secondsPerTick = 1.0 / double(ticksPerSecond.QuadPart);
  }
  return double(ticksDiff) * secondsPerTick;
#elif defined(FABRIC_OS_LINUX)
  return double(ticksDiff) * 1e-9;
#elif defined(FABRIC_OS_DARWIN)
  static mach_timebase_info_data_t sTimebaseInfo;
  if ( !sTimebaseInfo.denom )
    mach_timebase_info(&sTimebaseInfo);
  return double(ticksDiff) * double(sTimebaseInfo.numer) * 1e-9 / double(sTimebaseInfo.denom);
#else
# error "Unsupported OS"
#endif
}

inline double GetSecondsBetweenTicks( uint64_t start, uint64_t end )
{
  return GetSecondsForTicksDiff( end - start );
}

class ScopeTimer
{
public:
  
  ScopeTimer( char const *desc )
    : m_desc( desc )
    , m_begin( GetCurrentTicks() )
  {
  }

  ~ScopeTimer()
  {
    uint64_t end = GetCurrentTicks();
    double ms = GetSecondsBetweenTicks( m_begin, end ) * 1e3;
    printf( "%s %fms\n", m_desc, ms );
  }
  
private:
  
  char const *m_desc;
  uint64_t m_begin;
};

#endif
