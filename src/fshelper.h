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

#include <string>
#include <atomic>


namespace ZMFS {

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

    enum class Type{FS_VOID, FS_TEMP, FS_PERMANENT_LOCAL, FS_PERMANENT_REMOTE/*S3 or DB*/};

    explicit FSLocation();
    FSLocation(const FSLocation& other) = default;
    virtual ~FSLocation();

    /** Rssize given string and put the an absolute path.*/
    void absolute_path(std::string& str, const std::string& fname) const;

    FSLocation::Type type = FSLocation::Type::FS_VOID;
    std::string location; //< A path to a directory or remote URL
};

//---------------------------------------------------------
 
class FSItem
{
public:

  explicit FSItem(FSLocation locat = FSLocation());

  /** copy ctor will not copy on_destruction() callback, set it manually.*/
  FSItem(const FSItem& other);
  FSItem& operator = (const FSItem& other);

  FSItem(const std::string& file_name, FSLocation locat = FSLocation());

  FSLocation fslocation;
  std::string fname; //< in default constructor has 0-len, must be set.
  std::atomic_ulong fsize;
};
//---------------------------------------------------------
class FSHelper
{
public:
    FSHelper();
    virtual ~FSHelper();

    /** If empty, then it is initially set to values in ZMB::Settings.*/
    void set_dirs(const std::string& permanent_dir,
                  std::string temp_dir = std::string());

    /** Obtain full paths for temporary file creation.*/
    FSItem&& spawn_temp(const std::string& fname);

    /** Obtain full paths for permanent file creation.*/
    FSItem&& spawn_permanent(const std::string& fname);

    /** Move temporary file to permanent storage.
     * @param current item location.
     * @return new item values. */
    FSItem&& store_permanently(const FSItem* item);

    /** Remove the file if it's temporary.*/
    bool utilize(const FSItem& item);

    FSLocation temp_location;
    FSLocation perm_location;
};

}//ZMFS
//---------------------------------------------------------
#endif // FSHELPER_H
