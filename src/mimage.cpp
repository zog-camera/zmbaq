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
void SwsDeleter::operator()(struct SwsContext* ctx)
{
    if (ctx)
        sws_freeContext(ctx);
}
//-----------------------------------------------------------------------------
static void InitFromFrame(PictureHolder& ph, AVFrame* frame_p)
{
    ::memcpy(ph.dataSlicesArray.data(), frame_p->data, sizeof(frame_p->data));
    ::memcpy(ph.stridesArray.data(), frame_p->linesize, sizeof(frame_p->linesize));
    ph.format = frame_p->format;
}

PictureHolder::PictureHolder(PictureHolder&& rvalue) : PictureHolder()
{
    this->operator =(std::move(rvalue));
}

PictureHolder& PictureHolder::operator = (PictureHolder&& rvalue)
{
    frame_p.reset(rvalue.frame_p.release());
    if ( frame_p )
    {
        InitFromFrame(*this, frame_p.get());
    }
    else
    {
        dataSlicesArray = std::move(rvalue.dataSlicesArray);
        rvalue.dataSlicesArray.fill(0x00);
        stridesArray = std::move(rvalue.stridesArray);
        rvalue.stridesArray.fill(0x00);
    }
    format = rvalue.format; rvalue.format = -1;
    dimension = rvalue.dimension; rvalue.dimension = MSize(0,0);
    swsContextPtr.reset(rvalue.swsContextPtr.release());

    lastUsedDestDimension = rvalue.lastUsedDestDimension;
    rvalue.lastUsedDestDimension = MSize(0,0);

    lastUsedDestFormat = rvalue.lastUsedDestFormat;
    rvalue.lastUsedDestFormat = -1;
}

PictureHolder::PictureHolder(AVFrameUniquePtr&& frame) : PictureHolder()
{
    frame_p = std::move(frame);
    if ( frame_p )
        InitFromFrame(*this, frame_p.get());
}

int PictureHolder::fmt() const {return format;}
int PictureHolder::width() const {return dimension.width();}
int PictureHolder::height() const {return dimension.height();}


PictureHolder CreatePicture(MSize dim, int avpic_fmt)
{
    PictureHolder ph;
    int res = av_image_fill_linesizes(ph.stridesArray.data(), (enum AVPixelFormat)avpic_fmt, dim.width());
    if (res < 0)
        return std::move(ph);

    res = av_image_alloc(ph.dataSlicesArray.data(), ph.stridesArray.data(),
                         dim.width(), dim.height(),(enum AVPixelFormat)avpic_fmt, (int)sizeof(void*));
    if (res < 0)
    {
        return PictureHolder();
    }
    ph.dimension = dim;
    ph.format = avpic_fmt;
    return ph;
}

PictureHolder&& PictureHolder::Scale(int destination_av_fmt, MSize destinationDimensions)
{
    PictureHolder dst;
    dst.format = destination_av_fmt;
    dst.dimension = destinationDimensions;

    if ( nullptr == swsContextPtr
        || lastUsedDestFormat != destination_av_fmt
         || lastUsedDestDimension != destinationDimensions )
    {
        lastUsedDestFormat = destination_av_fmt;
        lastUsedDestDimension = destinationDimensions;
        swsContextPtr.reset ( sws_getContext(width(), height(), (enum AVPixelFormat)destination_av_fmt, //< src: {w,h,fmt}
                                             dst.width(), dst.height(), (enum AVPixelFormat)dst.fmt(), //< dst: {w,h,fmt}
                                             SWS_FAST_BILINEAR, NULL, NULL, NULL) );
    }
    sws_scale(swsContextPtr.get(),
              dataSlicesArray.data(), stridesArray.data(), //< src {data, strides}
              0/*src slice Y*/, h()/*src slice height*/,
              dst.dataSlicesArray.data(), dst.stridesArray.data() //< dst {data, strides}
              );
    return std::move(dst);
}
//-----------------------------------------------------------------------------


} //ZMB

