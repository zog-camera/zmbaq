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

FSItem::FSItem(FSLocation locat) : fslocation(locat)
{
}

FSItem::FSItem(const ZConstString& file_name, FSLocation locat)
    : fslocation(locat)
{
    fname = std::move(std::string(file_name.begin(), file_name.size()));
}

FSItem::FSItem(const FSItem& other)
{
  this->operator =(other);
}

FSItem& FSItem::operator = (const FSItem& other)
{
  fslocation = other.fslocation;
  fname = other.fname;
  fsize.store(other.fsize.load());
}

void FSItem::to_json(Json::Value* jo) const
{
    std::string buf;
    fslocation.absolute_path(buf, ZConstString(fname.data(), fname.size()));
    Json::Value& val(*jo);

    val["path"] = Json::Value(buf);
    val["filename"] = Json::Value(fname);
    val["location"] = Json::Value(fslocation.location);
    val["type"] = (int)fslocation.type;
}

bool FSItem::setLocation(FSHelper* helper)
{
  FSHelper& h(*helper);

  switch (fslocation.type) {
    case FSLocation::FS_TEMP:
      fslocation = h.temp_location;
      break;
    case FSLocation::FS_PERMANENT_REMOTE:
    case FSLocation::FS_PERMANENT_LOCAL:
      fslocation = h.perm_location;
      break;
    default:
      return false;
    }
  return true;
}

//-----------------------------------------------------------------------------
FSLocation::FSLocation()
{
    type = FS_TEMP;
#ifndef __WIN32
    location = "/tmp";
#else
    auto lk = Settings::instance()->get_locker();
    location = Settings::instance()->s_get_string(ZMB::ZMKW_USER_HOME, lk);
    location += "\\tmp";
#endif
}

FSLocation::~FSLocation()
{

}


void FSLocation::absolute_path(std::string& str, const ZConstString& fname) const
{
  assert(!fname.empty());
  str.reserve(location.size() + 1 + fname.size());
  str += location;
  str += dir_path_sep;
  str += fname.begin();
}

bool FSLocation::absolute_path(ZMBCommon::ZUnsafeBuf& str, const ZConstString& fname) const
{
  assert(!fname.empty());
  if (str.size() < (1 + fname.size() + location.size()) )
    { return false; }
  char* end = nullptr;
  str.read(&end, ZMBCommon::bindZCTo(location));
  *(end) = dir_path_sep; ++end;
  str.read(&end, fname, end);
  str.len = end - str.begin();
  return true;
}
// path = location + item_fname
static void make_item_abspath(std::string& path, const FSItem* item)
{
  item->fslocation.absolute_path(path, ZConstString(item->fname.data(), item->fname.size()));
}


FSHelper::FSHelper(Poco::Channel* logChannel)
{
  logChannelPtr = logChannel;
  ZMB::Settings* settings = ZMB::Settings::instance();

  auto settingsLk = settings->getLocker(); (void)settingsLk;

  //temporary storage:
  ZConstString path = settings->string(ZMB::ZMKW_FS_TEMP_LOCATION, settingsLk);
  temp_location.location = path.begin();
  //permanent storage:
  path = settings->string(ZMB::ZMKW_FS_PERM_LOCATION, settingsLk);
  perm_location.location = path.begin();

  perm_location.type = FSLocation::FS_PERMANENT_LOCAL;

}

FSHelper::~FSHelper()
{

}

void FSHelper::set_dirs(ZConstString permanent_dir, ZConstString temp_dir)
{
    static auto fn_make_path = [] (ZMFS::FSLocation& loc)
    {
       Poco::File dir(loc.location.data());
       dir.createDirectories();
    };
    if (!permanent_dir.empty())
    {
       perm_location.location = permanent_dir.begin();
    }
    if (!temp_dir.empty())
    {
        temp_location.location = temp_dir.begin();
    }

    fn_make_path(perm_location);
    fn_make_path(temp_location);
}


FSItem&& FSHelper::spawn_temp(const ZConstString& fname)
{
  return std::move(FSItem(fname, temp_location));
}

FSItem&& FSHelper::spawn_permanent(const ZConstString& fname)
{
  return std::move(FSItem(fname, perm_location));
}

FSItem&& FSHelper::store_permanently(const FSItem* item)
{
    FSItem res = FSItem(*item);

    std::string abs;
    make_item_abspath(abs, &res);

    Poco::File file(abs);//file ("/temp/location/file");
    if (!file.exists())
        return std::move(res);

    res.fslocation = perm_location;
    make_item_abspath(abs, &res);
    try {
         //("/temp/location/file") -> ("/storage/location/file)
        file.moveTo(abs);
    }catch (Poco::Exception& ex)
    {
      if (logChannelPtr)
        {
          Poco::Message msg;
          msg.setSource(std::string(__FUNCTION__));
          msg.setText(ex.message());
          logChannelPtr->log(msg);
        }
    }

    if (nullptr != sig_file_stored)
        sig_file_stored(*this, res);
    return std::move(res);
}

bool FSHelper::utilize(const FSItem* item)
{
    std::string abs;
    make_item_abspath(abs, item);

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
