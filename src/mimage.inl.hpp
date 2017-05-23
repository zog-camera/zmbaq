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


bool operator < (const ZMB::MRegion& lhs, const ZMB::MRegion& rhs)
{
    auto sl = lhs.square();
    auto sr = lhs.square();

    if (sl < sr)
        return true;
    else if (sl == sr)
        return lhs.left() < rhs.left();
    return false;
}

namespace ZMB {

bool MRegion::valid() const
{
    return 0 < width() * height();
}
int MRegion::width() const {return bottom_right().x - top_left().x;}
int MRegion::height() const {return top_left().y - bottom_right().y;}
int MRegion::square() const {return width() * height();}

glm::ivec2& MRegion::top_left()
{
    return this->operator [](0);
}

const glm::ivec2& MRegion::top_left() const
{
    return this->operator [](0);
}

glm::ivec2 MRegion::top_right() const
{
    return glm::ivec2(bottom_right().x, top_left().y);
}

glm::ivec2 MRegion::bottom_left() const
{
    return glm::ivec2(top_left().x, bottom_right().y);
}

const glm::ivec2& MRegion::bottom_right() const
{
    return this->operator [](1);
}

glm::ivec2& MRegion::bottom_right()
{
    return this->operator [](1);
}

int MRegion::left() const {return top_left().x;}

int MRegion::right() const {return bottom_right().x;}
int MRegion::top() const {return top_left().y;}
int MRegion::bottom() const {return bottom_right().y;}

//-----------------------------------------------------------------------------
void FrameDeleter::operator()(AVFrame* pframe) const
{
  av_frame_unref(pframe);
}
//-----------------------------------------------------------------------------
template<int MAX_SLICES_NUM>
PictureHolder<MAX_SLICES_NUM>::PictureHolder()
{
  dataSlicesArray.fill(nullptr); stridesArray.fill(0);
  format = -1;
  dimension = MSize(0,0);
}

template<int MAX_SLICES_NUM>
int PictureHolder<MAX_SLICES_NUM>::fmt() const {return format;}

template<int MAX_SLICES_NUM>
int PictureHolder<MAX_SLICES_NUM>::w() const {return dimension.width();}

template<int MAX_SLICES_NUM>
int PictureHolder<MAX_SLICES_NUM>::h() const {return dimension.height();}

//-----------------------------------------------------------------------------
template<class TPictureHolder, class TLockableObject, class TImp>
MImage<TPictureHolder, TLockableObject, TImp>::MImage()
{

}

template<class TPictureHolder, class TLockableObject, class TImp>
MImage<TPictureHolder, TLockableObject, TImp>::~MImage()
{
}

template<class TPictureHolder, class TLockableObject, class TImp>
TPictureHolder MImage<TPictureHolder, TLockableObject, TImp>::get() const
{
  return pv.picture;
}

template<class TPictureHolder, class TLockableObject, class TImp>
TPictureHolder MImage<TPictureHolder, TLockableObject, TImp>::getSafely(typename TLockableObject::Lock_t& lk) const
{
  return pv.picture;
}

template<class TPictureHolder, class TLockableObject, class TImp>
std::unique_ptr<AVFrame> MImage<TPictureHolder, TLockableObject, TImp>::extractFrame()
{
  return std::move(pv.frame_p);
}

template<class TPictureHolder, class TLockableObject, class TImp>
bool MImage<TPictureHolder, TLockableObject, TImp>::empty() const
{
   return nullptr == pv.picture;
}

template<class TPictureHolder, class TLockableObject, class TImp>
TPictureHolder MImage<TPictureHolder, TLockableObject, TImp>::create(MSize dim, int avpic_fmt)
{
  auto ph = TPictureHolder();
  int res = av_image_fill_linesizes(ph.stridesArray.data(), (enum AVPixelFormat)avpic_fmt, dim.width());
  if (res < 0)
    return nullptr;

  res = av_image_alloc(ph.dataSlicesArray.data(), ph.stridesArray.data(),
                       dim.width(), dim.height(),(enum AVPixelFormat)avpic_fmt, (int)sizeof(void*));
  if (res < 0)
    {
      return nullptr;
    }
  ph.onDestructCB = [](TPictureHolder* picture) {
    auto lk = picture->getLocker();
    av_freep(picture->dataSlicesArray.data());
  };
  ph.dimension = dim;
  ph.format = avpic_fmt;

  pv.picture = ph;
  return ph;
}
  
template<class TPictureHolder, class TLockableObject, class TImp>
TPictureHolder MImage<TPictureHolder, TLockableObject, TImp>::imbue(std::unique_ptr<AVFrame>& frame)
{
  pv.frame_p = std::move(frame);
  
  TPictureHolder& ph(pv.picture);
  ::memcpy(ph.dataSlicesArray.data(), frame->data, sizeof(frame->data));
  ::memcpy(ph.stridesArray.data(), frame->linesize, sizeof(frame->linesize));
  return pv.picture;
}

template<class TPictureHolder, class TLockableObject, class TImp>
void MImage<TPictureHolder, TLockableObject, TImp>::imbue(TPictureHolder&& picture)
{
  pv.picture = std::move(picture);
}

template<class TPictureHolder, class TLockableObject, class TImp>
TPictureHolder MImage<TPictureHolder, TLockableObject, TImp>::scale(TPictureHolder picture)
{
  TPictureHolder dst = picture;

  if(pv.lastCvtFmt != picture.fmt() || pv.lastCvtSize != picture.dimension)
    {
      pv.sws.reset();
      auto pRawSws = sws_getContext(picture.w(), picture.h(), (enum AVPixelFormat)picture.fmt(),
                               dst.w(), dst.h(), (enum AVPixelFormat)dst.fmt(), SWS_FAST_BILINEAR, NULL, NULL, NULL);;
      pv.sws = std::move(SwsObj(pRawSws));
    }
  sws_scale(pv.sws.get(), picture.dataSlicesArray.data(), picture.stridesArray.data(),
            0, picture.h(),
            dst.dataSlicesArray.data(), dst.stridesArray.data());
  return pv.picture;
}

} //ZMB

