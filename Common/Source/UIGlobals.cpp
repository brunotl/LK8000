#include "UIGlobals.hpp"

#ifndef WIN32
#include "Window/WndMain.h"

SingleWindow &
UIGlobals::GetMainWindow()
{
  return (*main_window);
}
#endif