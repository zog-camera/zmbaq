/*A video surveillance software with support of H264 video sources.
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
along with this program.  If not, see <http://www.gnu.org/licenses/>.*/


#include "mbytearray.h"
#include <assert.h>
#include <arpa/inet.h>
#include <cstring>
#include "json/writer.h"

namespace ZMBCommon {

MByteArray::MByteArray() : base_type  ()
{

}

MByteArray::MByteArray(const std::string& str) : base_type  ()
{
    if (!str.empty())
    {
        resize(str.size(), 0x00);
        memcpy(unsafe(), str.c_str(), str.size());
    }
}

MByteArray::MByteArray(const ZConstString& zstr) : base_type  ()
{
    if (zstr.size() == 0)
        return;

    resize(zstr.size(), 0x00);
    memcpy(unsafe(), zstr.begin(), zstr.size());
}

MByteArray::MByteArray(const char* str) : base_type ()
{
}
#ifdef WITH_QT5
MByteArray::MByteArray(const QString& qstr)
    : MByteArray(qstr.toUtf8())
{
}
MByteArray::MByteArray(const QByteArray& bytes)
    : base_type (bytes.data(), (size_type)bytes.size(), CharAllocatorSingleton::instance()->c_allocator())
{
}
#endif //qt5


MByteArray::MByteArray(const char* str, base_type::size_type pos, base_type::size_type count)
    : base_type (str, pos, count)
{
}

MByteArray::MByteArray(const MByteArray& bytes) : base_type ()
{
    if (bytes.size() > 0)
    {
        this->resize(bytes.size());
        ::memcpy(unsafe(), bytes.data(), size());
    }
}

MByteArray::~MByteArray()
{
}

char* MByteArray::unsafe() const
{
    return const_cast<char*>(data());
}

char* MByteArray::unsafe_end() const
{
   return unsafe() + size();
}

MByteArray& MByteArray::replace(const char& one, const char& another)
{
   for (auto it = begin(); it != end(); ++it)
   {
       if (*it == one)
       {
           *it = another;
       }
   }
   return *this;
}

MByteArray& MByteArray::operator = (const MByteArray& other)
{
    if (other.size() > 0)
    {
        resize(other.size());
        memcpy(unsafe(), other.data(), size());
    }
    return *this;
}
#ifdef WITH_QT5
MByteArray& MByteArray::operator = (const QByteArray& other)
{
    if (other.size() > 0)
    {
        resize(other.size());
        memcpy(unsafe(), other.data(), size());
    }
    return *this;
}

MByteArray& MByteArray::operator = (const QString& other)
{
    *this = other.toUtf8();
    return *this;
}

MByteArray& MByteArray::operator += (const QByteArray& other)
{
    *this += ZConstString(other.data(), other.size());
    return *this;
}

QString MByteArray::to_qstring() const
{
    return QString(make_qbyte_depend(*this));
}
QByteArray MByteArray::to_barr() const
{
    return QByteArray(data(), size());
}
#endif //qt5

MByteArray& MByteArray::operator = (const char* str)
{
    resize(strlen(str));
    if (size() > 0)
        memcpy(unsafe(), str, size());
    return *this;
}

MByteArray& MByteArray::operator += (const MByteArray& other)
{
    *this += ZConstString(other.data(), other.size());
    return *this;
}

MByteArray& MByteArray::operator += (char c)
{
    *this += ZConstString(&c, 1);
    return *this;
}

MByteArray& MByteArray::operator += (const char* str)
{
   append(str);
   return *this;
}

MByteArray& MByteArray::operator += (const ZConstString& str)
{
    if (str.size() == 0)
        return *this;
    char* backup = 0;

    size_t old_sz = size();
    bool on_stack = old_sz < 4096;
    resize(old_sz + str.size(), 0x00);
    if (old_sz > 0)
    {
        backup = on_stack? (char*)alloca(old_sz) : new char[old_sz];
        memcpy(backup, unsafe(), old_sz);
    }
    memcpy(unsafe() + old_sz, str.begin(), str.size());

    if (!on_stack && old_sz > 0)
    {
        memcpy(unsafe(), backup, old_sz);
        delete[] backup;
    }

    return *this;
}


ZConstString MByteArray::get_const_str() const
{
   return ZConstString(data(), size());
}



Json::Value MByteArray::to_jvalue() const
{
    return Json::Value(data());
}

std::string MByteArray::toStdString() const
{
   return std::string(data(), size());
}

int MByteArray::toInt() const
{
   return atoi(data());
}

std::shared_ptr<MByteArrayList> MByteArray::split(const char& c) const
{
    auto lst = std::make_shared<MByteArrayList>();

    char* saveptr1 = 0;
    char delim[2] = {c, 0x00};

    MByteArray clone = *this;
    char* ptr = clone.unsafe();

    for (char* token = ptr; nullptr != token; ptr = nullptr)
    {
        token = strtok_r(ptr, delim, &saveptr1);
        if (nullptr != token)
            lst->push_back(MByteArray(token));
    }
    return lst;
}

void MByteArray::fill(char c)
{
    if (!empty())
    {
        memset(unsafe(), c, size());
    }
}

MByteArray MByteArray::substr(size_type pos, size_type n) const
{
    return *substr_ptr(pos, n);
}

std::shared_ptr<MByteArray> MByteArray::substr_ptr(size_type pos, size_type n) const
{
    auto res = std::make_shared<MByteArray>();

    if (0 == n || pos >= size())
        return res;

    if (npos == n)
    {
        n = size() - pos;
    }

    if ((pos + n) <= size())
    {
        res->resize(n, 0x00);
        if (1 == n)
            res->operator[](0) = at(pos);
        else
            memcpy(res->unsafe(), data(), n);
    }
    return res;
}
#ifdef WITH_QT5
QByteArray make_qbyte_depend(const MByteArray& arr)
{
    QByteArray bytes;
    bytes.setRawData(arr.data(), arr.size());
    return bytes;
}

QLatin1String make_qlatin1_depend(const MByteArray& arr)
{
   return QLatin1String(arr.data(), arr.size());
}
#endif //WITH_QT5


MByteArray& MByteArray::operator << (const ZConstString& other)
{
    this->operator +=(other);
    return *this;
}

MByteArrayList::MByteArrayList() : base_type()
{

}
MByteArrayList& MByteArrayList::operator << (const char* str)
{
    MByteArray arr = str;
    push_back(arr);
    return *this;
}

MByteArrayList& MByteArrayList::operator << (const MByteArray& str)
{
    push_back(str);
    return *this;
}
//================



std::string jexport(const Json::Value* val)
{
    Json::FastWriter wr;
    return wr.write(*val);
}

}





//-------------------------------
ZMBCommon::MByteArray operator + (const ZMBCommon::MByteArray& one, const ZMBCommon::ZConstString& another)
{
   ZMBCommon::MByteArray sum;
   sum += another;
   return sum;
}

bool operator == (const ZMBCommon::ZConstString& str, const ZMBCommon::MByteArray& arr)
{
  return str.size() == arr.size() && 0 == memcmp(str.begin(), arr.data(), str.size());
}

bool operator == (const ZMBCommon::ZUnsafeBuf& str, const ZMBCommon::MByteArray& arr)
{
  return ZMBCommon::ZConstString((const char*)str.begin(), str.size()) == arr.get_const_str();
}
//-------------------------------
bool operator < (const ZMBCommon::MByteArray& one, const ZMBCommon::MByteArray& another)
{
  return one.size() <  another.size()?
        true : ((one.size() > another.size() || one == another)?
                  false : true );
}


#undef HN_CONVERT_BODY

