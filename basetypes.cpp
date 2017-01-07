#include "stdafx.h"
#include "basetypes.h"
#include "utils.h"

void Handle::swap(Handle &other)
{
	using std::swap;
	swap(this->value, other.value);
}


Handle::operator void *() const
{
	return this->value;
}

Handle & Handle::operator=(Handle other)
{
	return other.swap(*this), *this;
}

Handle::~Handle()
{
	if (valid(this->value)) {
		CloseHandle(value);
	}
}

Handle::Handle(Handle const &other) : value(other.value)
{
	if (valid(this->value)) {
		CheckAndThrow(DuplicateHandle(GetCurrentProcess(), this->value, GetCurrentProcess(),
			&this->value, MAXIMUM_ALLOWED, TRUE, DUPLICATE_SAME_ACCESS));
	}
}

Handle::Handle(void *const value) : value(value)
{
	if (!valid(value)) {
		throw std::invalid_argument("invalid handle");
	}
}

Handle::Handle() : value()
{

}

bool Handle::valid(void *const value)
{
	return value && value != reinterpret_cast<void *>(-1);
}

