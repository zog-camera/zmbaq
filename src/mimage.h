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


#ifndef MIMAGE_H
#define MIMAGE_H
#include <memory>
#include <glm/vec2.hpp>
#include <glm/mat2x2.hpp>
#include <mutex>
#include "boost/noncopyable.hpp"
#include "./minor/lockable.hpp"

extern "C"
{
#include "libavutil/frame.h"
#include "libavutil/imgutils.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
}


namespace ZMB {

struct MSize : public glm::ivec2
{
  MSize() : glm::ivec2(-1, -1) { }
  MSize(int _w, int _h) : glm::ivec2(_w, _h) { }
  
  const int& width() const {return x;}
  const int& height() const {return y;}
  int square() const {return x * y;}
};

/** An image region, made using GLM 2x2 iteger matrix as base structure.
  @verbatim
          col0   col1
  idx 0 [ x1    x2   ]
  idx 1 [ y1    y2   ]
  @endverbatim
 */
struct MRegion : public glm::detail::tmat2x2<int, glm::highp>
{
  MRegion() : type(col_type(-1, 1), col_type(1, -1))
  { }

  bool valid() const;
  int width() const;
  int height() const;
  int square() const;

  glm::ivec2& top_left();
  const glm::ivec2& top_left() const;

  glm::ivec2 top_right() const ;

  glm::ivec2 bottom_left() const ;

  const glm::ivec2& bottom_right() const ;
  glm::ivec2& bottom_right();

  int left() const;
  int right() const;
  int top() const;
  int bottom() const;

  //    /*inherited:*/
  //    col_type& operator [](size_t index);

};

bool operator < (const ZMB::MRegion& lhs, const ZMB::MRegion& rhs);

//-----------------------------------------------------------------------------
/** Default deleter (av_frame_unref(frame)).*/
struct FrameDeleter
{
  //makes av_frame_unref(pframe)
  void operator()(AVFrame* pframe) const;
};

template<int MAX_SLICES_NUM = 8>
class PictureHolder
{
public:
  //will zero-fill the pointers.
  PictureHolder();
  PictureHolder(const PictureHolder&) = default;
  PictureHolder& operator = (const PictureHolder&) = default;
  
  PictureHolder(PictureHolder&& rvalue)
  {
    dataSlicesArray = std::move(rvalue.dataSlicesArray);
    stridesArray = std::move(rvalue.stridesArray);
    format = rvalue.format; dimension = rvalue.dimension;
    rvalue.format = -1; rvalue.dimension = MSize(-1,-1);
  }

  int fmt() const;
  int w() const;
  int h() const;

  std::array<uint8_t*, MAX_SLICES_NUM> dataSlicesArray;
  std::array<int, MAX_SLICES_NUM> stridesArray;
  int format;//< constructed as -1, NONE; casted from (enum AVPixelFormat)
  MSize dimension;
    
};

//-----------------------------------------------------------------------------
class MImagePV;

/** 
 * @tparam TPictureHolder -- you can replace the simple type with something having a destructor etc.
 * @tparam TImp -- just a pointer-less private implementation parameter.
*/
 template<class TPictureHolder = PictureHolder<8>,
   class TLockableObject = ZMB::LockableObject,
   class TImp = MImagePV>
class MImage : public boost::noncopyable
{
  TImp pv;

public:
  //--------------------------------------------------  
  TLockableObject syncObject;//< sync object: using mutex or dull stub (depends on TLockableObject)
  typename TLockableObject::Mutex_t& mutex() { return syncObject.mutex();}

  //for example: auto lk = mimage.getLocker();
  typename TLockableObject::Lock_t&& getLocker() const { return syncObject(); }
  //--------------------------------------------------
  
  MImage();
  virtual ~MImage();
  
  bool empty() const;
  
  //non thread-safe access:
  TPictureHolder get() const;

  //thread-safe access:
  TPictureHolder getSafely(typename TLockableObject::Lock_t& lk) const;

  /** @return move out the AVFrame pointer previously was imbued().*/
  std::unique_ptr<AVFrame> extactFrame();

  /** Alloc new image data of given dimensions and format. */
  TPictureHolder create(MSize dim, /*(AVPixelFormat)*/int avpic_fmt);

  /** Take ownership of the frame data and/or frame pointer.
    * The user must provide custom deleter.
    * @return picture pointers constructed from the frame.
    */
  TPictureHolder imbue(std::unique_ptr<AVFrame>& frame);

  /** Take ownership of the picture data.
   */
  void imbue(TPictureHolder&& picture);

  /** Make scaled data from other picture */
  TPictureHolder scale(TPictureHolder picture);

  //
  std::unique_ptr<AVFrame> extractFrame();
};
//-----------------------------------------------------------------------------
 class SwsObj : public boost::noncopyable
 {
 public:
   SwsObj(struct SwsContext* pSws = nullptr) : sws(pSws) { }
   virtual ~SwsObj() { if (sws) sws_freeContext(sws); }

   SwsObj(SwsObj&& rhs) { sws = rhs.sws; rhs.sws = nullptr; }
   
   struct SwsContext* get() const { return sws; }
   
   struct SwsContext* sws;
 };

class MImagePV
{
public:
    MImagePV()
    {
      lastCvtFmt = -1;
      lastCvtSize = MSize(0,0);
    }

    PictureHolder<8> picture;
    MSize lastCvtSize;
    int lastCvtFmt;
    
    //mutex is exposed in MImage for locking
    std::mutex mu;
    
    //this field used only when you explicitly MImage::imbue(frame) with a deleter
    std::unique_ptr<AVFrame> frame_p;
    
    //used for re-scaling
    SwsObj sws;
};
}//ZMB

#include "mimage.inl.hpp"

#endif // MIMAGE_H
