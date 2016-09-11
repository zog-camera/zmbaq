/*
A video surveillance software with support of H264 video sources.
Copyright (C) 2015 Bogdan Maslowsky, Alexander Sorvilov. 

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef ZMB_STR_H
#define ZMB_STR_H

#include "json/value.h"
#include <exception>

#ifndef ZM_DEF_STATIC_CONST_STRING
    #define ZM_DEF_STATIC_CONST_STRING(VAR_NAME, STR) \
    static const ZMBCommon::ZConstString (VAR_NAME)((STR), sizeof(STR) - 1, ZMBCommon::ZConstString::Type::STATIC_CONST);

    #define ZCSTR(CONST_STRING) ZMBCommon::ZConstString((CONST_STRING), sizeof(CONST_STRING) - 1, ZMBCommon::ZConstString::Type::STATIC_CONST)

#endif //ZM_DEF_STATIC_CONST_STRING

namespace ZMBCommon {


//----
struct ZConstString
{
    enum Type{STATIC_CONST, STATIC, UNSPECIFIED};

    ZConstString() { len = 0; str = 0; type = Type::UNSPECIFIED;}

    ZConstString(const char* a_str);
    ZConstString(const char* a_str, size_t a_len, Type tp = Type::UNSPECIFIED);
    ZConstString(const char* begin, const char* end, Type tp = Type::UNSPECIFIED);

    //get string for jobject[key.str], empty if not found.
    ZConstString(const ZConstString& key, const Json::Value* jobject);

    /** Checks whether this string is a valid key in the JSON object.
     * @param: p_jvalue: pointer to the JSON value in (jobject) passed here (if not 0).
     * @return if the string is a key of JSON. */
    bool is_key_of(const Json::Value* jobject, const Json::Value** p_jvalue = nullptr) const;

    inline const char* last() const {return str + len - 1;}
    inline const char* begin() const {return str;}
    //next element after the last one (0-char usually)
    inline const char* end() const {return str + len;}
    inline size_t size() const {return len;}
    inline bool empty() const {return len == 0;}

    bool has_prefix(const ZConstString& prefix_str) const;

    /** memcpy string to the destination.
     * @param dest: reading destination.
     * @param n_bytes: size() bytes written if npos == n_bytes, n_bytes otherwise. */
    void* write(void* dest, size_t n_bytes = std::string::npos) const;

    const char* str;
    size_t len;

    Type type;

};
//----
struct ZUnsafeBuf
{
    ZUnsafeBuf();

    ZUnsafeBuf(char* a_str);

    ZUnsafeBuf(char* a_str, size_t a_len);
    ZUnsafeBuf(char* b, char* e);
    virtual ~ZUnsafeBuf();

    ZConstString to_const() const;

    inline char* last() const {return str + len - 1;}
    inline char* begin() const {return str;}
    //next element after the last one (0-char usually)
    inline char* end() const {return str + len;}

    inline const char* cbegin() const {return (const char*)begin();}
    inline const char* cend() const {return (const char*)end();}
    inline size_t size() const {return len;}

    void fill(int val);

    /** Make memcmp(begin(), other.begin(), min_size);*/
    bool is_equal(const ZConstString& other) const;
    bool has_prefix(const ZConstString& prefix_str) const;

    /** Copy other string data, limit capacity to fit to the end.
     * Throws an error if has not enough of space for copy.
     * @param start position of the destination(this) buffer,
     * if NULL we use begin();
 */
    void imbue(const ZConstString& other, char* dest_start = nullptr);

    /** Write data and set pointer to the data end.
     *  @param end_ptr: if != 0, will point to the next element after the last one (STL iterator style)
     *  @return false if buffer overflow is possible. */
    bool read(char** end_ptr, const ZConstString& data, char* start_pos = NULL);

    //Copy with converting to network byte order, the data muyst be aligned.
    bool read_htonl(u_int32_t** end_ptr, const ZConstString& data, u_int32_t* start_pos = NULL);
    bool read_htons(u_int16_t** end_ptr, const ZConstString& data, u_int16_t* start_pos = NULL);

    bool read_ntohl(u_int32_t** end_ptr, const ZConstString& data, u_int32_t* start_pos = NULL);
    bool read_ntohs(u_int16_t** end_ptr, const ZConstString& data, u_int16_t* start_pos = NULL);

    /** Copy values while converting them to hex format: 0xaabbccdd  in ASCII.
     * Notice: size() must be greater than: (data.size() * 4) / 10; */
    bool transform_hex4b(char** end_ptr, const ZConstString& data, char* start_pos = NULL);

    /** Read data and convert to binary. input format 0xaabbccdd in ASCII*/
    bool from_hex4b(char** end_ptr, const ZConstString& hex_data, char* start_pos = NULL);

    /** htonl() for a region of buffer.
     * @return false if out of bounds. */
    bool htonl_self(u_int32_t* begin_ptr, u_int32_t* end_ptr = NULL);

    /** htons() for a region of buffer.
     * @return false if out of bounds. */
    bool htons_self(u_int16_t* begin_ptr, u_int16_t* end_ptr = NULL);

    bool ntohl_self(u_int32_t* begin_ptr, u_int32_t* end_ptr = NULL);
    bool ntohs_self(u_int16_t* begin_ptr, u_int16_t* end_ptr = NULL);

    char* str;
    size_t len;

    void* sidedata;//for use with care

};

}//namespace ZMBCommon
///----------------------------------------------------------------------------
bool operator == (const ZMBCommon::ZConstString& first, const ZMBCommon::ZConstString& second);
bool operator != (const ZMBCommon::ZConstString& first, const ZMBCommon::ZConstString& second);
///----------------------------------------------------------------------------

#endif // ZMB_STR_H
