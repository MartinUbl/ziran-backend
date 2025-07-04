#include "guid.h"

#include <random>
#include <array>

GUID Generate_GUIDv4()
{
    GUID guid;

    static std::random_device rdev;

    guid.Data1 = static_cast<decltype(guid.Data1)>(rdev());
    guid.Data2 = static_cast<decltype(guid.Data2)>(rdev());
    guid.Data3 = static_cast<decltype(guid.Data3)>(rdev());

    for (size_t i = 0; i < 8; i++)
        guid.Data4[i] = static_cast<std::remove_reference_t<decltype(guid.Data4[0])>>(rdev());

    return guid;
}

using TGUID_Overlay = uint8_t[16];
static_assert(sizeof(GUID) == sizeof(TGUID_Overlay), "Invalid packing of GUID overlay!");
const std::array<uint8_t, 16> guid_overlay_indexes = { 3, 2, 1, 0, 5, 4, 7, 6, 8, 9, 10, 11, 12, 13, 14, 15 };	//UUID would start with 0, 1, 2, 3
const std::array<size_t, 16> guid_parsing_indexes = { 0, 2, 4, 6, 9, 11, 14, 16, 19, 21, 24, 26, 28, 30, 32, 34 };

GUID String_To_GUID(const std::string& str, bool& ok)
{
    GUID guid = Invalid_GUID;

    const size_t bracketless_guid_len = 36;
    const size_t str_len = str.size();
    const bool bracketed_guid = str_len == bracketless_guid_len + 2;
    ok = (str_len == bracketless_guid_len) || bracketed_guid;

    if (ok)
    {
        //guid str seems to have the proper size
        ok = !bracketed_guid || (bracketed_guid && (str[0] == L'{') && (str[bracketless_guid_len + 1] == L'}'));
        if (ok)
        {
            //if brackets are used, they are OK

            //check the hyphens
            const char* gs = str.data() + (bracketed_guid ? 1 : 0);
            const size_t h1st = 8;
            const size_t h2nd = 13;
            const size_t h3rd = 18;
            const size_t h4th = 23;
            ok = (gs[h1st] == L'-') && (gs[h2nd] == L'-') && (gs[h3rd] == L'-') && (gs[h4th] == L'-');
            if (ok)
            {
                auto hex_2_bin = [&ok](const wchar_t ch)->uint8_t {
                    switch (ch) {
                        case '0': case '1': case '2': case '3':	case '4': case '5':	case '6': case '7': case '8': case '9': return ch - L'0';
                        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': return ch - L'a' + 10;
                        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': return ch - L'A' + 10;
                        default: ok = false; return 0xFF;
                    }
                };

                TGUID_Overlay& guid_overlay = *reinterpret_cast<TGUID_Overlay*>(&guid);
                for (size_t i = 0; i < guid_parsing_indexes.size(); i++)
                {
                    const size_t idx = guid_parsing_indexes[i];
                    guid_overlay[guid_overlay_indexes[i]] = hex_2_bin(gs[idx + 0]) * 16 + hex_2_bin(gs[idx + 1]);
                }

                if (!ok)
                    guid = Invalid_GUID;	//undo any writes that might have occured
            }
        }
    }


    return guid;
}

std::string GUID_To_String(const GUID& guid)
{
    std::string result = "{00000000-0000-0000-0000-000000000000}";
    const char* to_x = "0123456789ABCDEF";

    TGUID_Overlay& guid_overlay = *reinterpret_cast<TGUID_Overlay*>(const_cast<GUID*>(&guid));

    for (size_t i = 0; i < guid_parsing_indexes.size(); i++)
    {
        const uint8_t byte = guid_overlay[guid_overlay_indexes[i]];
        const uint8_t lo = byte & 0xf;
        const uint8_t hi = byte / 0x10;

        const size_t result_index = guid_parsing_indexes[i];
        result[result_index + 1] = to_x[hi];        //+1 because of the starting '{'
        result[result_index + 2] = to_x[lo];
    }

    return result;
}
