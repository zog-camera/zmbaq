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


#ifndef MBYTEARRAY_H
#define MBYTEARRAY_H
#include <vector>
#include <string>
#include <memory>
#ifdef WITH_QT5
#include <QByteArray>
#include <QLatin1String>
#include <QString>
#endif

#include <list>
#include <deque>
#include "zmb_str.h"

namespace ZMBCommon {

class MByteArrayList;

class MByteArray : public std::basic_string<char>
{
public:
    typedef std::basic_string<char> base_type;
    MByteArray();

    MByteArray(const char* str);
    MByteArray(const char* str, base_type::size_type pos, base_type::size_type count = std::basic_string<char>::npos);
    MByteArray(const ZConstString& zstr);

    MByteArray(const std::string& str);
    MByteArray(const MByteArray& bytes);

    virtual ~MByteArray();

#ifdef WITH_QT5
    //makes data copy and ensured to be 0-terminated::
    MByteArray(const QByteArray& bytes);
    MByteArray(const QString& qstr);

    //makes deep copy:
    QByteArray to_barr() const;
    QString to_qstring() const;

    MByteArray& operator = (const QByteArray& other);
    MByteArray& operator = (const QString& other);
    MByteArray& operator += (const QByteArray& other);

#endif


    char* unsafe() const;

    //next char in memory after end.
    char* unsafe_end() const;

    void fill(char c = 0x00);
    int toInt() const;

    MByteArray substr(size_type pos = 0, size_type n = npos) const;
    std::shared_ptr<MByteArray> substr_ptr(size_type pos = 0, size_type n = npos) const;

    //get ZConstString to access data read-only:
    ZConstString get_const_str() const;

    Json::Value to_jvalue() const;
    std::string toStdString() const;

    MByteArray& replace(const char& one, const char& another);

    //split by separator char
    std::shared_ptr<MByteArrayList> split(const char& c) const;

    //makes data copy:
    MByteArray& operator = (const MByteArray& other);
    MByteArray& operator = (const char* str);

    //append
    MByteArray& operator += (const char* str);
    MByteArray& operator += (char c);
    MByteArray& operator += (const ZConstString& str);
    MByteArray& operator += (const MByteArray& other);

    MByteArray& operator << (const ZConstString& other);

};


#ifdef WITH_QT5

/** Create QByteArray object that only has a copy of pointer
that is stored in MByteArray.
@return the object is only valid while (arr) exists;
*/
QByteArray make_qbyte_depend(const MByteArray& arr);

/** Create QLatin1String object that only has a copy of pointer
that is stored in MByteArray.
@return the object is only valid while (arr) exists;
*/
QLatin1String make_qlatin1_depend(const MByteArray& arr);
#endif


class MByteArrayList : public std::deque<MByteArray>
{
public:
    typedef std::deque<MByteArray> base_type;
    explicit MByteArrayList();

    MByteArrayList& operator<<(const char* str);
    MByteArrayList& operator<<(const MByteArray& str);

};


typedef std::shared_ptr<MByteArray> MByteArrayPtr;
}//ZMBCommon


//bool operator == (const char* str, const MByteArray& arr);
bool operator == (const ZMBCommon::ZUnsafeBuf& str, const ZMBCommon::MByteArray& arr);
bool operator == (const ZMBCommon::ZConstString& str, const ZMBCommon::MByteArray& arr);
ZMBCommon::MByteArray operator + (const ZMBCommon::MByteArray& one, const ZMBCommon::ZConstString& another);
bool operator < (const ZMBCommon::MByteArray& one, const ZMBCommon::MByteArray& another);


#endif // MBYTEARRAY_H
