#include "KronosActivity.h"

// Kept apart from the JUCE headers because the Windows and CoreGraphics
// headers declare names that clash with them.
#if defined(__APPLE__)
 #include <CoreGraphics/CoreGraphics.h>
#elif defined(_WIN32)
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
#endif

namespace KronosActivity
{
double getSecondsSinceLastUserInput()
{
   #if defined(__APPLE__)
    return CGEventSourceSecondsSinceLastEventType(kCGEventSourceStateCombinedSessionState, kCGAnyInputEventType);
   #elif defined(_WIN32)
    LASTINPUTINFO info {};
    info.cbSize = sizeof(info);

    if (! GetLastInputInfo(&info))
        return 0.0;

    // Unsigned subtraction stays correct when the tick count wraps
    return static_cast<double>(GetTickCount() - info.dwTime) / 1000.0;
   #else
    return 0.0;
   #endif
}
}
