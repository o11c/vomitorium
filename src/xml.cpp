#include "xml.hpp"

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <algorithm>


XmlOutput::XmlOutput(FILE *out, bool close)
: output_file(out)
, should_close(close)
, in_tag(false)
, in_attribute(false)
, start_of_line(true)
, soft_newline(false)
, current_indent(0)
, indent_spaces(2) // or 4, we don't actually indent much.
{
    if (!out)
        abort();

    this->emit_raw("<?xml version=\"1.0\" encoding=\"ascii\"?>", 38);
    this->emit_newline();
}

XmlOutput::~XmlOutput()
{
    assert (!this->in_tag);
    assert (!this->in_attribute);
    assert (this->current_indent == 0);
    this->flush();
    if (this->should_close)
        fclose(this->output_file);
    else
        fflush(this->output_file);
}

void XmlOutput::flush()
{
    if (this->in_tag)
    {
        assert (!this->start_of_line);
        this->in_tag = false;
        this->emit_raw(">", 1);
    }
    if (this->start_of_line)
    {
        this->start_of_line = false;
        this->emit_spaces(this->current_indent * this->indent_spaces);
    }
}

void XmlOutput::emit_spaces(size_t n)
{
    static const char many_spaces[] = "                                ";
    while (n)
    {
        size_t l = std::min(n, strlen(many_spaces));
        size_t rv = fwrite(many_spaces, 1, l, this->output_file);
        if (rv <= 0)
            abort();
        n -= rv;
    }
}

void XmlOutput::emit_raw(const char *s, size_t len)
{
    this->flush();
    while (len)
    {
        size_t rv = fwrite(s, 1, len, this->output_file);
        if (rv == 0)
            abort();
        s += rv;
        len -= rv;
    }
}

void XmlOutput::emit_newline()
{
    this->emit_raw("\n", 1);
    this->start_of_line = true;
    this->soft_newline = false;
}

void XmlOutput::emit_string(const char *s)
{
    assert (s && "Can't emit_string(NULL)!");
    for (size_t i = 0; s[i]; ++i)
    {
        assert (' ' <= s[i] && s[i] <= '~' && "Can't emit a non-ASCII-printable string");
    }

    while (true)
    {
        size_t len = strcspn(s, "<>&\"");
        if (len)
        {
            this->emit_raw(s, len);
            s += len;
        }

        while (true)
        {
            switch (*s)
            {
            case '<':
                this->emit_raw("&lt;", 4);
                ++s;
                continue;
            case '>':
                this->emit_raw("&gt;", 4);
                ++s;
                continue;
            case '&':
                this->emit_raw("&amp;", 5);
                ++s;
                continue;
            case '"':
                this->emit_raw("&quot;", 6);
                ++s;
                continue;
            case '\0':
                return;
            }
            break;
        }
    }
}


XmlTag::XmlTag(XmlOutput *o, const char *t)
: out(o)
, tag(t)
{
    // out->in_tag may be true or false; out->flush() will handle it.
    assert (!out->in_attribute);

    if (out->soft_newline)
        out->emit_newline();
    else
        out->flush();

    out->emit_raw("<", 1);
    out->emit_string(this->tag);

    out->in_tag = true; // > or /> depending on whether anything happens
    out->soft_newline = true;

    out->current_indent += 1;
}

XmlTag::~XmlTag()
{
    if (!out)
        return;

    assert (!out->in_attribute);

    out->current_indent -= 1;
    if (out->in_tag)
    {
        out->in_tag = false;
        out->emit_raw("/>", 2);
    }
    else
    {
        out->emit_raw("</", 2);
        out->emit_string(this->tag);
        out->emit_raw(">", 1);
    }
    out->emit_newline();
}

XmlAttr::XmlAttr(XmlOutput *o, const char *a)
: out(o)
{
    assert (out->in_tag);
    assert (!out->in_attribute);
    out->in_tag = false;
    out->in_attribute = true;

    out->emit_raw(" ", 1);
    out->emit_string(a);
    out->emit_raw("=\"", 2);
}

XmlAttr::~XmlAttr()
{
    if (!out)
        return;

    assert (!out->in_tag);
    assert (out->in_attribute);

    out->emit_raw("\"", 1);

    out->in_attribute = false;
    out->in_tag = true;
}
