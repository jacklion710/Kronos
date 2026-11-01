#pragma once

namespace KronosActivity
{
// Seconds since the last mouse, keyboard or trackpad input anywhere on the
// computer. Hosts do not report user activity, so this comes from the OS.
double getSecondsSinceLastUserInput();
}
