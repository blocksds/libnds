// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2025 Antonio Niño Díaz

#include <stddef.h>

#include <nds/ndstypes.h>
#include <nds/utf.h>

ssize_t utf16_to_utf8(char *out, size_t out_size, char16_t *in)
{
    ssize_t ret = 0;
    ssize_t out_len = 0;

    while (1)
    {
        char32_t codepoint;

        // Decode UTF-16
        // -------------

        // https://datatracker.ietf.org/doc/html/rfc2781

        char16_t w1 = *in++;

        if ((w1 <= 0xD7FF) || (w1 >= 0xE000)) // NUL is included here
        {
            codepoint = w1;
        }
        else
        {
            if ((w1 >= 0xD800) && (w1 <= 0xDBFF))
            {
                char16_t w2 = *in; // Don't advance pointer yet
                if ((w2 < 0xDC00) || (w1 > 0xDFFF))
                {
                    codepoint = ((w1 & 0x3FF) << 10) | (w2 & 0x3FF);
                    in++;
                }
                else // Values outside of that range or NUL characters
                {
                    codepoint = UTF_REPLACEMENT_CHARACTER;
                    ret = -1;
                }
            }
            else
            {
                codepoint = UTF_REPLACEMENT_CHARACTER;
                ret = -1;
            }
        }

        // Encode UTF-8
        // ------------

        // https://en.wikipedia.org/wiki/UTF-8#Description

        char utf8[4];
        int utf8_len;

        if (codepoint > 0x10FFFF)
        {
            codepoint = UTF_REPLACEMENT_CHARACTER;
            ret = -1;
        }
        else if (codepoint <= 0x7F)
        {
            utf8[0] = codepoint & 0x7F;
            utf8_len = 1;
        }
        else if (codepoint <= 0x7FF)
        {
            utf8[0] = 0xC0 | ((codepoint >> 6) & 0x1F);
            utf8[1] = 0x80 | (codepoint & 0x3F);
            utf8_len = 2;
        }
        else if (codepoint <= 0xFFFF)
        {
            utf8[0] = 0xE0 | ((codepoint >> 12) & 0xF);
            utf8[1] = 0x80 | ((codepoint >> 6) & 0x3F);
            utf8[2] = 0x80 | (codepoint & 0x3F);
            utf8_len = 3;
        }
        else if (codepoint <= 0x10FFFF)
        {
            utf8[0] = 0xF0 | ((codepoint >> 18) & 0x7);
            utf8[1] = 0x80 | ((codepoint >> 12) & 0x3F);
            utf8[2] = 0x80 | ((codepoint >> 6) & 0x3F);
            utf8[3] = 0x80 | (codepoint & 0x3F);
            utf8_len = 4;
        }

        // Save to destination
        // -------------------

        for (int i = 0; i < utf8_len; i++)
        {
            if (out_size == 0)
                break;

            *out = utf8[i];
            out++;
            out_size--;
            out_len++;
        }

        // Check if we're done

        if (codepoint == 0)
            break;
    }

    if (ret != 0)
        return ret;
    else
        return out_len;
}
