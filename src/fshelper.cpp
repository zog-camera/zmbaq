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
#include <assert.h>
#include <boost/filesystem.hpp>
#include <cstdlib>
#include <iostream>

namespace ZMFS {

  FSItem::FSItem(FSLocation locat) : fslocation(locat)
  {
  }

  FSItem::FSItem(const std::string& file_name, FSLocation locat)
    : fslocation(locat)
  {
    fname = file_name;
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


//-----------------------------------------------------------------------------
  FSLocation::FSLocation()
  {
    type = FSLocation::Type::FS_TEMP;
#if _WIN32 || WIN64
    location = std::getenv("TMP");
#else
    location = "/tmp";
#endif
  }

  FSLocation::~FSLocation()
  {

  }

  void FSLocation::absolute_path(std::string& str, const std::string& fname) const
  {
    assert(!fname.empty());
    str.reserve(location.size() + 1 + fname.size());
    str += location;
    str += dir_path_sep;
    str += fname;
  }


  FSHelper::FSHelper()
  {
//permanent storage:
    std::string home(std::getenv("HOME"));
    perm_location.location = std::getenv("HOME");
    perm_location.location += "Videos";
    perm_location.type = FSLocation::Type::FS_PERMANENT_LOCAL;
  }

  FSHelper::~FSHelper()
  {

  }

  void FSHelper::set_dirs(const std::string& permanent_dir,  std::string temp_dir)
  {
    auto fn_make_path = [] (ZMFS::FSLocation& loc)
      {
        boost::filesystem::path dir(loc.location);
        try
        {
          boost::filesystem::create_directory(dir);
        }
        catch(boost::filesystem::filesystem_error& ex)
        {
          std::cerr << ex.what() << std::endl;
        }
      };
    assert(!temp_dir.empty());
    assert(!permanent_dir.empty());
    
    perm_location.location = permanent_dir;
    temp_location.location = temp_dir;

    fn_make_path(perm_location);
    fn_make_path(temp_location);
  }


  FSItem&& FSHelper::spawn_temp(const std::string& fname)
  {
    return std::move(FSItem(fname, temp_location));
  }

  FSItem&& FSHelper::spawn_permanent(const std::string& fname)
  {
    return std::move(FSItem(fname, perm_location));
  }

// path = location + item_fname
  static void make_item_abspath(std::string& path, const FSItem& item)
  {
    item.fslocation.absolute_path(path, item.fname);
  }

  FSItem&& FSHelper::store_permanently(const FSItem* item)
  {
    FSItem res = FSItem(*item);

    std::string abs;
    make_item_abspath(abs, res);
    using namespace boost::filesystem;
    
    path file(abs);//file ("/temp/location/file");
    assert(is_regular_file(file));
    res.fslocation = perm_location;
    make_item_abspath(abs, res);
    
    try {
      //move file ("/temp/location/file") -> ("/storage/location/file)
      rename(file, abs);
    }catch (filesystem_error& ex)
    {
      std::cerr << ex.what() << std::endl;
    }
    return std::move(res);
  }

  bool FSHelper::utilize(const FSItem& item)
  {
    using namespace boost::filesystem;
    std::string abs;
    make_item_abspath(abs, item);

    path file(abs);//file ("/temp/location/file");
    try {
      //remove
      remove(file);
    }
    catch (filesystem_error& ex)
    {
      std::cerr << ex.what() << std::endl;
      return false;
    }
    return true;
  }
  
} //namespace ZMFS
