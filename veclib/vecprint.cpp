#include "vecprint.h"
using namespace std;

ostream& operator << (ostream& out, vec3f v)
{
	out << "(" << v.x() << ", " << v.y() << ", " << v.z() << ")";
	return out;
}

ostream& operator << (ostream& out, vec4f v)
{
	out << "(" << v.x() << ", " << v.y() << ", " << v.z() << ", " << v.w() << ")";
	return out;
}

ostream& operator << (ostream& out, quatf v)
{
	out << "(" << v.x() << ", " << v.y() << ", " << v.z() << ", " << v.w() << ")";
	return out;
}

///< Matrix printing helper function.
ostream& operator << (ostream& out, const mat4f& m)
{
	for (unsigned i = 0; i < 4; i++)
	{
		out << m.element()[i] << " " << m.element()[i + 4] << " " << m.element()[i + 8]
				<< " " << m.element()[i + 12] << endl;
	}
	return out;
}

///< Matrix printing helper function.
ostream& operator << (ostream& out, const mat3f& m)
{
	for (unsigned i = 0; i < 4; i++)
	{
		out << m.element()[i] << " " << m.element()[i + 4] << " " << m.element()[i + 8]
				<< " " << m.element()[i + 12] << endl;
	}
	return out;
}
