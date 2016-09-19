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


#include "fshelper.h"
#include <Poco/File.h>
#include <Poco/Exception.h>
#include <assert.h>
#include "Poco/Message.h"
#include <Poco/DirectoryIterator.h>

namespace ZMFS {

FSItem::FSItem(SHP(FSLocation) locat)
    : fslocation(locat)
{
}

FSItem::FSItem(const ZConstString& file_name, SHP(FSLocation) locat)
    : fslocation(locat)
{
    fname = std::move(std::string(file_name.begin(), file_name.size()));
}

FSItem::FSItem(const FSItem& other)
{
  fslocation = other.fslocation;
  fname = other.fname;
  fsize.store(other.fsize.load());
}

FSItem::~FSItem()
{
  if(nullptr != on_destruction)
    {
      on_destruction();
    }
  if (0u < fname.size() && nullptr != fslocation
      && FSLocation::FS_TEMP == fslocation->type)
    {
      fslocation->fshelper->utilize(this);
    }
}

void FSItem::to_json(Json::Value* jo)
{
    fslocation->mut.lock();
    ZUnsafeBuf buf(fslocation->temp, sizeof(fslocation->temp));
    std::string* pbuf = 0;
    if (buf.size() < 1 + fname.size() + fslocation->location.size())
    {
        pbuf = new std::string();
        fslocation->absolute_path(*pbuf, ZConstString(fname.data(), fname.size()));
        buf.str = &pbuf->operator [](0);
        buf.len = pbuf->size();
    }
    else
    {
        fslocation->absolute_path(&buf, ZConstString(fname.data(), fname.size()));
    }
    Json::Value& val(*jo);

    val["path"] = Json::Value(buf.begin(), buf.end());
    val["filename"] = Json::Value(fname);
    auto location_p = fslocation->location.data();
    val["location"] = Json::Value(location_p, location_p + fslocation->location.size());
    val["type"] = (int)fslocation->type;
    delete pbuf;
    fslocation->mut.unlock();
}

FSLocation::FSLocation(std::shared_ptr<ZMFS::FSHelper> master)
    : fshelper(master)
{
    type = FS_TEMP;
#ifndef __WIN32
    location = ZCSTR("/tmp");
#else
    auto lk = Settings::instance()->get_locker();
    location = Settings::instance()->s_get_string(ZMB::ZMKW_USER_HOME, lk);
    location += ZCSTR("\\tmp");
#endif
}

FSLocation::~FSLocation()
{

}


void FSLocation::absolute_path(std::string& str, const ZConstString& fname)
{
    str.resize(location.size() + 1 + fname.size());
    ZUnsafeBuf bf(&str[0], str.size());
    char* end = 0;
    bf.read(&end, location.get_const_str());
    *end = dir_path_sep; ++end;
    bf.read(&end, fname, end);
}

bool FSLocation::absolute_path(ZUnsafeBuf* str, const ZConstString& fname)
{
    if (str->size() < (1 + fname.size() + location.size()) )
    {
        str->str = 0;
        str->len = 0;
        return false;
    }
    char* end = nullptr;
    str->read(&end, location.get_const_str());
    *(end) = dir_path_sep; ++end;
    str->read(&end, fname, end);
    str->len = end - str->begin();
    return true;
}

class FSHelper::FSPV
{
public:
    FSPV()
    {
      logChannel = nullptr;
      temp_location = std::make_shared<FSLocation>();
      perm_location = std::make_shared<FSLocation>();
      ZMB::Settings* settings = ZMB::Settings::instance();
      std::unique_lock<std::mutex> lk(settings->accessMutex);
      ZConstString path = settings->string(ZMB::ZMKW_FS_TEMP_LOCATION, lk);
      temp_location->location = path;
      path = settings->string(ZMB::ZMKW_FS_PERM_LOCATION, lk);
      perm_location->location = path;
      perm_location->type = FSLocation::FS_PERMANENT_LOCAL;
    }
    ~FSPV()
    {

    }
    // path = location + item_fname
    void make_item_abspath(std::string& path, const FSItem* item)
    {
        assert(0 != item->fslocation.get());
        item->fslocation->absolute_path(path, ZConstString(item->fname.data(), item->fname.size()));
    }
    void assign_item_location(FSItem* item)
    {
        switch (item->fslocation->type) {
        case FSLocation::FS_TEMP:
            item->fslocation = temp_location;
            break;
        case FSLocation::FS_PERMANENT_REMOTE:
        case FSLocation::FS_PERMANENT_LOCAL:
            item->fslocation = perm_location;
            break;
        default:
            break;
        }
    }
    Poco::Channel* logChannel;
    std::shared_ptr<FSHelper> master;
    SHP(FSLocation) temp_location;
    SHP(FSLocation) perm_location;
};

FSHelper::FSHelper(Poco::Channel* logChannel) : pv(std::make_shared<FSPV>())
{
  pv->logChannel = logChannel;
}

FSHelper::~FSHelper()
{

}

void FSHelper::set_dirs(ZConstString permanent_dir, ZConstString temp_dir)
{
    static auto fn_make_path = [] (ZMFS::FSLocation* loc)
    {
       Poco::File dir(loc->location.data());
       dir.createDirectories();
    };
    if (!permanent_dir.empty())
    {
       pv->perm_location->location = permanent_dir;
    }
    if (!temp_dir.empty())
    {
        pv->temp_location->location = temp_dir;
    }

    fn_make_path(pv->perm_location.get());
    fn_make_path(pv->temp_location.get());
    pv->perm_location->fshelper = shared_from_this();
    pv->temp_location->fshelper = shared_from_this();
}


SHP(FSItem) FSHelper::spawn_temp(const ZConstString& fname)
{
  pv->master = shared_from_this();
  return std::make_shared<FSItem>(pv->temp_location);
}

SHP(FSItem) FSHelper::spawn_permanent(const ZConstString& fname)
{
  pv->master  = shared_from_this();
  return std::make_shared<FSItem>(fname, pv->perm_location);
}

SHP(FSItem) FSHelper::store_permanently(const FSItem* item)
{
    SHP(FSItem) res = std::make_shared<FSItem>(*item);

    std::string abs;
    pv->make_item_abspath(abs, res.get());

    Poco::File file(abs);//file ("/temp/location/file");
    if (!file.exists())
        return res;

    res->fslocation = pv->perm_location;
    pv->make_item_abspath(abs, res.get());
    try {
         //("/temp/location/file") -> ("/storage/location/file)
        file.moveTo(abs);
    }catch (Poco::Exception& ex)
    {
      if (pv->logChannel)
        {
          Poco::Message msg;
          msg.setSource(std::string(__FUNCTION__));
          msg.setText(ex.message());
          pv->logChannel->log(msg);
        }
    }

    if (nullptr != sig_file_stored)
        sig_file_stored(shared_from_this(), res);
    return res;
}

bool FSHelper::utilize(const FSItem* item)
{
    std::string abs;
    pv->make_item_abspath(abs, item);

    Poco::File file(abs);
    bool ok = false;
    if (file.exists())
    {
        ok = true;
        file.remove();
    }
    return ok;
}

} //namespace ZMFS
