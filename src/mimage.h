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

extern "C" {
    struct AVFrame;
    struct AVPicture;
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
struct MRegion : public glm::tmat2x2<int, glm::highp>
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
/** Default deleter.*/
struct FrameDeleter
{
  //makes av_frame_unref(pframe)
  void operator()(AVFrame* pframe) const;
};

class PictureHolder
{
public:
  static const int MAX_SLICES_NUM = 8;
  typedef std::mutex Mutex_t;
  typedef std::unique_lock<Mutex_t> Lock_t;

  //will zero-fill the pointers.
  PictureHolder();

  //calls onDestructCB()
  virtual ~PictureHolder();

  int fmt() const;
  int w() const;
  int h() const;


  std::array<uint8_t*, MAX_SLICES_NUM> dataSlicesArray;
  std::array<int, MAX_SLICES_NUM> stridesArray;
  int format;//< constructed as -1, NONE; casted from (enum AVPixelFormat)
  MSize dimension;

  //synchronization mutex:
  Mutex_t mu;

  //for example: auto lk = mimage->getLocker(); //binded to this->mu
  Lock_t&& getLocker();

  std::function<void(PictureHolder*)> onDestructCB;
};
typedef std::shared_ptr<PictureHolder> PictureHolderPtr;
//-----------------------------------------------------------------------------
/** Notice! Copying of the object -- is sharing.
 *
*/
class MImage
{
  class MImagePV;
  std::shared_ptr<MImagePV> pv;

public:
  //--------------------------------------------------
  typedef std::mutex Mutex_t;
  class Lock_t : public std::unique_lock<Mutex_t>
  {
  public:
    Lock_t(Mutex_t& mu, std::shared_ptr<MImagePV> ref)
      : std::unique_lock<Mutex_t>(mu), refptr(ref)
    {}

    std::shared_ptr<MImagePV> refptr;
  };
  //--------------------------------------------------
  MImage();
  virtual ~MImage();

  bool empty() const;

  Mutex_t& mutex() const;

  //for example: auto lk = mimage->getLocker();
  Lock_t&& getLocker() const;

  //non thread-safe access:
  PictureHolderPtr get() const;

  //thread-safe access:
  PictureHolderPtr getSafe(Lock_t& lk) const;

  /** @return non-NULL pointer only when the AVFrame pointer was imbued().*/
  std::shared_ptr<AVFrame> getFrame(Lock_t&);

  /** Alloc new image data of given dimensions and format. */
  PictureHolderPtr create(MSize dim, /*(AVPixelFormat)*/int avpic_fmt, Lock_t&);

  /** Take ownership of the frame data and/or frame pointer.
    * The user must provide custom deleter.
    * @return picture pointers constructed from the frame.
    */
  PictureHolderPtr imbue(std::shared_ptr<AVFrame> frame, Lock_t&);

  /** Take ownership of the picture data.
   *  The shared pointer can be constructed with custom deleter
   * or without it but with PictureHolder.onDestructCB callback instead.
   */
  void imbue(PictureHolderPtr picture, Lock_t&);

  /** Make scaled data from other picture */
  PictureHolderPtr scale(PictureHolderPtr picture, Lock_t&);
};
//-----------------------------------------------------------------------------

}//ZMB
#endif // MIMAGE_H
