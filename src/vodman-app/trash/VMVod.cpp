#include "VMVod.h"



VMVod::~VMVod()
{
}

VMVod::VMVod()
    : d(new VMVodData())
{
}

VMVod::VMVod(const VMVod& other)
    : d(other.d)
{
}

VMVod&
VMVod::operator=(const VMVod& other)
{
    VMVod(other).swap(*this);
    return *this;
}

void
VMVod::swap(VMVod& other) {
    d.swap(other.d);
}
