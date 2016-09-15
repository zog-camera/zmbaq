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


#ifndef FSHELPER_H
#define FSHELPER_H
#include "zmbaq_common/zmbaq_common.h"
#include "zmbaq_common/mbytearray.h"
#include <atomic>
#include "Poco/Channel.h"


namespace ZMFS {

using namespace ZMBCommon;

class FSHelper;

ZM_DEF_STATIC_CONST_STRING(KW_VIDEO_FILE, "video_file")
ZM_DEF_STATIC_CONST_STRING(KW_SCR_FILE, "screenshot_file")

//---------------------------------------------------------
class FSLocation
{
public:
    static const char dir_path_sep =
#ifndef __WIN32
    '/'
#else
    '\\';
#endif
    ;

    enum Type{FS_VOID, FS_TEMP, FS_PERMANENT_LOCAL, FS_PERMANENT_REMOTE/*S3 or DB*/};

    FSLocation(FSHelper* master);
    virtual ~FSLocation();

    /** Rssize given string and put the an absolute path.*/
    void absolute_path(std::string& str, const ZConstString& fname);

    /** The provided unsafe buffer must contain enough space!*/
    bool absolute_path(ZUnsafeBuf* str, const ZConstString& fname);

    SHP(FSHelper) fshelper;
    Type type;
    ZMBCommon::MByteArray location; //< A path to a directory or remote URL

    std::mutex mut;
    char temp[1024];
};

//---------------------------------------------------------
class FSItem
{
public:

  explicit FSItem(SHP(FSLocation) locat = 0);

  /** will not copy on_destruction() callback, set it manually.*/
  FSItem(const FSItem& other);


  FSItem(const ZConstString& file_name, SHP(FSLocation) locat = 0);

  virtual ~FSItem();

  std::function<void()> on_destruction;

  /** Exports these fields to Json::Value:
     *  jo["path"] = "full path";
     *  jo["filename"] = "filename";
     *  jo["location"] = "/location";
     *  jo["type"] = "db" || "local" || "remote"; */
  void to_json(Json::Value* jo);

  SHP(FSLocation) fslocation;
  std::string fname; //< in default constructor has 0-len, must be set.
  std::atomic_ulong fsize;
};
//---------------------------------------------------------
class FSHelper : public std::enable_shared_from_this<FSHelper>
{
public:
    std::function<void(SHP(FSHelper), SHP(FSItem))> sig_file_stored;

    explicit FSHelper(Poco::Channel* logChannel = nullptr);
    virtual ~FSHelper();

    /** If empty, then it is initially set to values in ZMB::Settings.*/
    void set_dirs(ZConstString permanent_dir = ZConstString(),
                  ZConstString temp_dir = ZConstString());

    /** Obtain full paths for temporary file creation.*/
    SHP(FSItem) spawn_temp(const ZConstString& fname);

    /** Obtain full paths for permanent file creation.*/
    SHP(FSItem) spawn_permanent(const ZConstString& fname);

    /** Move temporary file to permanent storage.
     * @param current item location.
     * @return new item values. */
    SHP(FSItem) store_permanently(const FSItem* item);

    /** Remove the file if it's temporary.*/
    bool utilize(const FSItem* item);

private:
    class FSPV;
    SHP(FSPV) pv;
};

}//ZMFS
//---------------------------------------------------------
#endif // FSHELPER_H
