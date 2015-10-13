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
#include "libavcodec/avcodec.h"
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



class MImage::MImagePV
{
public:
    MImagePV()
    {
       fn_allocate_frame = []() {return av_frame_alloc();};
       fn_deallocate_frame = [](AVFrame** ppframe) {av_frame_free(ppframe); *ppframe = 0;};
       fn_avpicture_alloc = [](AVPicture* picture, int pix_fmt,
                      int width, int height) -> int
       {
           return avpicture_alloc(picture, (enum AVPixelFormat)pix_fmt, width, height);
       };
       fn_avpicture_free = [](AVPicture* pic) {avpicture_free(pic);};
       frame_p = 0;

       memset(&picture, 0, sizeof(picture));
       format = -1;
    }
    ~MImagePV()
    {
        clear();
    }
    void clear()
    {
        fn_avpicture_free(&picture);
        fn_deallocate_frame(&frame_p);
        format = -1;
        sz = MSize(-1,-1);
    }

    AVFrame* frame_p;
    AVPicture picture;
    int format;
    MSize sz;

    std::function<AVFrame*()> fn_allocate_frame;
    std::function<void(AVFrame** ppframe)> fn_deallocate_frame;

    std::function<int(AVPicture* /*picture*/, int /*enum AVPixelFormat pix_fmt*/,
                      int /*width*/, int /*height)*/)
                      > fn_avpicture_alloc;
    std::function<void (AVPicture *picture)> fn_avpicture_free;

};

MImage::MImage() : pv(std::make_shared<MImagePV>())
{

}

MImage::~MImage()
{
}

bool MImage::empty() const
{
   return pv->sz.width() > 0 && pv->sz.height() > 0;
}

void MImage::clear()
{
    pv->clear();
}

bool MImage::create(MSize dim, int avpic_fmt)
{
    AVPicture neu;
    memset(&neu, 0, sizeof(neu));

    int res = pv->fn_avpicture_alloc(&neu, (AVPixelFormat)avpic_fmt, dim.width(), dim.height());
    if (0 == res)
        imbue(neu);
    return (0 == res);
}

bool MImage::imbue(AVFrame* frame)
{
    clear();
    pv->frame_p = frame;
    pv->format = frame->format;
    pv->sz = MSize(frame->width, frame->height);
    return true;
}

bool MImage::imbue(const AVPicture &pic, int format, MSize dim)
{
    clear();
    pv->sz = dim;
    pv->picture = pic;
    pv->format = format;
    return true;
}

int MImage::fmt() const
{
    return pv->format;
}

MSize MImage::dimensions() const
{
    return pv->sz;
}
int MImage::w() const
{
    return pv->sz.width();
}

int MImage::h() const
{
    return pv->sz.height();
}


AVPicture MImage::unsafe_access() const
{
    if (nullptr == pv->frame_p)
    {
       return pv->picture;
    }
    AVPicture pc;
    memcpy(&pc.data, &(pv->frame_p->data), sizeof(pc.data));
    memcpy(&pc.linesize, &(pv->frame_p->linesize), sizeof(pc.linesize));
    return pc;
}



std::function<AVFrame*()>* MImage::pfn_allocate_frame() const
{
        return &(pv->fn_allocate_frame);
}

std::function<void(AVFrame** ppframe)>* MImage::pfn_deallocate_frame() const
{
    return &(pv->fn_deallocate_frame);
}

std::function<int(AVPicture* /*picture*/, int /*enum AVPixelFormat pix_fmt*/,
                  int /*width*/, int /*height)*/) >*
MImage::pfn_avpicture_alloc() const
{
        return &(pv->fn_avpicture_alloc);
}

std::function<void (AVPicture *picture)>* MImage::pfn_avpicture_free() const
{
    return &(pv->fn_avpicture_free);
}

} //ZMB

