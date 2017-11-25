// No #pragma once; we are only included by the .hpp file.

#include <utility>

#include "compat.hpp"


template<class F>
void XmlOutput::with_output_file(F&& f)
{
    this->flush();
    std::forward<F>(f)(this->output_file);
}
inline XmlTag XmlOutput::tag(const char *t)
{
    return XmlTag(this, t);
}
inline XmlAttr XmlOutput::attr(const char *a)
{
    return XmlAttr(this, a);
}

inline XmlTag::XmlTag(XmlTag&& other)
{
    this->out = other.out; other.out = nullptr;
    this->tag = other.tag; other.tag = nullptr;
}
inline XmlTag& XmlTag::operator = (XmlTag&& other)
{
    std::swap(this->out, other.out);
    std::swap(this->tag, other.tag);
    return *this;
}

inline XmlAttr::XmlAttr(XmlAttr&& other)
{
    this->out = other.out; other.out = nullptr;
}
inline XmlAttr& XmlAttr::operator = (XmlAttr&& other)
{
    std::swap(this->out, other.out);
    return *this;
}
