#pragma once

/*
    Easy-to-use, pretty XML output.

    ASCII-only; always escapes < > & "
    Uses SBRM; allows arbitrary (but safe) emit()s in attribute values.
    Automatically generates self-closing tags when applicable.
    Ensures a newline before every opening tag and after every closing tag.
    (but no newline around text content)

    Because some tools lose the order, never use more than 1 attribute.
*/
#include <cstdio>

class XmlTag;
class XmlAttr;

class XmlOutput
{
    friend class XmlTag;
    friend class XmlAttr;

    FILE *output_file;
    bool should_close : 1;

    bool in_tag : 1;
    bool in_attribute : 1;

    bool start_of_line : 1;
    bool soft_newline : 1;

    size_t current_indent;
    size_t indent_spaces;
public:
    XmlOutput(FILE *out, bool should_close);
    XmlOutput(const XmlOutput&) = delete;
    XmlOutput& operator = (const XmlOutput&) = delete;
    ~XmlOutput();

    void flush();

    void emit_spaces(size_t n);
    void emit_raw(const char *s, size_t len);
    void emit_newline();
    void emit_string(const char *s);

    template<class F>
    void with_output_file(F&& f);

    XmlTag tag(const char *t);
    XmlAttr attr(const char *a);
};

class XmlTag
{
    friend class XmlOutput;

    XmlOutput *out;
    const char *tag;
protected:
    XmlTag(XmlOutput *out, const char *tag);
public:
    XmlTag(XmlTag&&);
    XmlTag& operator = (XmlTag&&);
    ~XmlTag();
};

class XmlAttr
{
    friend class XmlOutput;

    XmlOutput *out;
protected:
    XmlAttr(XmlOutput *out, const char *attr);
public:
    XmlAttr(XmlAttr&&);
    XmlAttr& operator = (XmlAttr&&);
    ~XmlAttr();
};


#include "xml.tcc"
