#include <Device/Mouse.hpp>

using namespace Cass;

long	Mouse::posX = -1;
long	Mouse::posY = -1;
int		Mouse::wheelDelta = 0;
bool	Mouse::pressedLB = false;
bool	Mouse::pressedMB = false;
bool	Mouse::pressedRB = false;