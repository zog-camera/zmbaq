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
#include <array>
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
/** Default deleter (av_frame_unref(frame)).*/
struct FrameDeleter
{
    //makes av_frame_unref(pframe)
    void operator()(AVFrame* pframe) const;
};
typedef std::unique_ptr<AVFrame, FrameDeleter> AVFrameUniquePtr;
//-----------------------------------------------------------------------------
struct SwsDeleter
{
    void operator()(struct SwsContext* ctx);
};
typedef std::unique_ptr<struct SwsContext, SwsDeleter> SwsUniquePtr;
//-----------------------------------------------------------------------------
class PictureHolder : public boost::noncopyable
{
public:
    static constexpr int MAX_SLICES_NUM = 8;
    //will zero-fill the pointers.

    PictureHolder(const PictureHolder&) = delete;
    PictureHolder& operator = (const PictureHolder&) = delete;
    PictureHolder()
    {
        stridesArray.fill(0x00);
        dataSlicesArray.fill(0x00);
    }

    PictureHolder(PictureHolder&& rvalue);

    //take ownership of the frame, also fills the (dataSlircesArray) struct to point to frame's internals
    PictureHolder(AVFrameUniquePtr&& frame);

    virtual ~PictureHolder()
    {
        if ( nullptr == frame_p )
        {
            av_freep(dataSlicesArray.data());
            dataSlicesArray.fill(0x00);
        }

    }


    int fmt() const;
    int w() const { return width(); }
    int width() const;
    int h() const { return height(); }
    int height() const;

    // convert current picture into required format/dimensions
    PictureHolder&& Scale(int destination_av_fmt, MSize destinationDimensions);



    std::array<uint8_t*, MAX_SLICES_NUM> dataSlicesArray;
    std::array<int, MAX_SLICES_NUM> stridesArray;
    int format = -1;//< constructed as -1, NONE; casted from (enum AVPixelFormat)
    MSize dimension = MSize(0, 0);

    //this field used only when you explicitly MImage::imbue(frame) with a deleter
    AVFrameUniquePtr frame_p;

    //used for conversion
    SwsUniquePtr swsContextPtr;
    int lastUsedDestFormat = -1;
    MSize lastUsedDestDimension = MSize(0, 0);
};
//-----------------------------------------------------------------------------
/** Alloc new image data of given dimensions and format. */
PictureHolder CreatePicture(MSize dim, /*(AVPixelFormat)*/int avpic_fmt);

/** Make scaled data from other picture */
PictureHolder ScalePicture(const PictureHolder& picture, SwsUniquePtr& rSwsContext);
//-----------------------------------------------------------------------------

}//ZMB


#endif // MIMAGE_H
