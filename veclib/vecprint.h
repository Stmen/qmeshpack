#include "veclib.h"
#include <iostream>
#include <ostream>

std::ostream& operator << (std::ostream& out, vec3f v);
std::ostream& operator << (std::ostream& out, vec4f v);
std::ostream& operator << (std::ostream& out, quatf v);
std::ostream& operator << (std::ostream& out, const mat3f& m);
std::ostream& operator << (std::ostream& out, const mat4f& m);
