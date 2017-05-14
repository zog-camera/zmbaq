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
#include <boost/filesystem.hpp
#include <cstdlib>

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
#if _WIN32 || WIN64
  temp_location.location = std::getenv("TMP");
#else
  temp_location.location = "/tmp";
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
  perm_location.type = FSLocation::FS_PERMANENT_LOCAL;
}

FSHelper::~FSHelper()
{

}

void FSHelper::set_dirs(const std::string& permanent_dir, const std::string&temp_dir)
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
static void make_item_abspath(std::string& path, const FSItem* item)
{
  item->fslocation.absolute_path(path, fname);
}

FSItem&& FSHelper::store_permanently(const FSItem* item)
{
    FSItem res = FSItem(*item);

    std::string abs;
    make_item_abspath(abs, &res);
    using BFS = boost::filesystem;
    
    BFS::path file(abs);//file ("/temp/location/file");
    assert(BFS::is_regular_file(file));
    res.fslocation = perm_location;
    make_item_abspath(abs, &res);
    
    try {
      //move file ("/temp/location/file") -> ("/storage/location/file)
      BFS::rename(file, abs);
    }catch (BFS::filesystem_error& ex)
      {
	std::cerr << ex.what() << std::endl;
      }
    return std::move(res);
}

  bool FSHelper::utilize(const FSItem* item)
{
    std::string abs;
    make_item_abspath(abs, item);

    BFS::path file(abs);//file ("/temp/location/file");
    try {
      //remove
      BFS::remove(file);
    }
    catch (BFS::filesystem_error& ex)
      {
	std::cerr << ex.what() << std::endl;
	return false;
      }
    return true;
}

} //namespace ZMFS
