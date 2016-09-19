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


#include "mimage.h"

extern "C"
{
#include "libavutil/frame.h"
#include "libavutil/imgutils.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
}

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
PictureHolder::PictureHolder()
{
  dataSlicesArray.fill(nullptr); stridesArray.fill(0);
  format = -1;
  dimension = MSize(0,0);
}
PictureHolder::~PictureHolder()
{
  if (nullptr != onDestructCB)
    {
      onDestructCB(this);
    }
}

PictureHolder::Lock_t&& PictureHolder::getLocker()
{
  return std::move(PictureHolder::Lock_t(mu));
}

int PictureHolder::fmt() const {return format;}
int PictureHolder::w() const {return dimension.width();}
int PictureHolder::h() const {return dimension.height();}

//-----------------------------------------------------------------------------

class MImage::MImagePV
{
public:
    MImagePV()
    {
      sws = nullptr;
      lastCvtFmt = -1;
      lastCvtSize = MSize(0,0);
    }
    ~MImagePV()
    {
    }

    std::shared_ptr<AVFrame> frame_p;
    std::shared_ptr<PictureHolder> picture;
    struct SwsContext* sws;
    MSize lastCvtSize;
    int lastCvtFmt;
    std::mutex mu;
};

MImage::MImage() : pv(std::make_shared<MImagePV>())
{

}

MImage::~MImage()
{
}

MImage::Mutex_t& MImage::mutex() const
{
  return pv->mu;
}

MImage::Lock_t&& MImage::getLocker() const
{
  return std::move(MImage::Lock_t(mutex(), pv));
}

PictureHolderPtr MImage::get() const
{
  return pv->picture;
}

PictureHolderPtr MImage::getSafe(Lock_t& lk) const
{
  return pv->picture;
}

std::shared_ptr<AVFrame> MImage::getFrame(Lock_t&)
{
  return pv->frame_p;
}

bool MImage::empty() const
{
   return nullptr == pv->picture;
}

std::shared_ptr<PictureHolder> MImage::create(MSize dim, int avpic_fmt, Lock_t&)
{
  auto ph = std::make_shared<PictureHolder>();
  int res = av_image_fill_linesizes(ph->stridesArray.data(), (enum AVPixelFormat)avpic_fmt, dim.width());
  if (res < 0)
    return nullptr;

  res = av_image_alloc(ph->dataSlicesArray.data(), ph->stridesArray.data(),
                       dim.width(), dim.height(),(enum AVPixelFormat)avpic_fmt, (int)sizeof(void*));
  if (res < 0)
    {
      return nullptr;
    }
  ph->onDestructCB = [](PictureHolder* picture) {
    auto lk = picture->getLocker();
    av_freep(picture->dataSlicesArray.data());
  };
  ph->dimension = dim;
  ph->format = avpic_fmt;

  pv->picture = ph;
  return ph;
}

PictureHolderPtr MImage::imbue(std::shared_ptr<AVFrame> frame, Lock_t&)
{
  auto ph = std::make_shared<PictureHolder>();
  ::memcpy(ph->dataSlicesArray.data(), frame->data, sizeof(frame->data));
  ::memcpy(ph->stridesArray.data(), frame->linesize, sizeof(frame->linesize));
  ph->onDestructCB = [frame](PictureHolder*) { (void)frame;};
  pv->picture = ph;
  return ph;
}

void MImage::imbue(std::shared_ptr<PictureHolder> picture, Lock_t&)
{
  pv->picture = picture;
}

PictureHolderPtr MImage::scale(PictureHolderPtr picture, Lock_t&)
{
  auto srclk = picture->getLocker();
  auto dstlk = getLocker();

  if (nullptr == pv->picture)
    return pv->picture;

  PictureHolder& dst(*(pv->picture.get()));

  if(pv->lastCvtFmt != picture->fmt() || pv->lastCvtSize != picture->dimension)
    {
      if(nullptr != pv->sws)
        sws_freeContext(pv->sws);
      pv->sws = sws_getContext(picture->w(), picture->h(), (enum AVPixelFormat)picture->fmt(),
                               dst.w(), dst.h(), (enum AVPixelFormat)dst.fmt(), SWS_FAST_BILINEAR, NULL, NULL, NULL);
    }
  sws_scale(pv->sws, picture->dataSlicesArray.data(), picture->stridesArray.data(),
            0, picture->h(),
            dst.dataSlicesArray.data(), dst.stridesArray.data());
  return pv->picture;
}

} //ZMB

