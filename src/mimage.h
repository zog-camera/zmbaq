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


/** Notice! Copying of the object -- is sharing.
 *
*/
class MImage
{
public:
    std::function<AVFrame*()>* pfn_allocate_frame() const;
    std::function<void(AVFrame** ppframe)>* pfn_deallocate_frame() const;

    std::function<int(AVPicture* /*picture*/, int /*enum AVPixelFormat pix_fmt*/,
                      int /*width*/, int /*height)*/)
                      >* pfn_avpicture_alloc() const;
    std::function<void (AVPicture *picture)>* pfn_avpicture_free() const;


    MImage();
    virtual ~MImage();

    bool empty() const;

    void clear(); //< deallocates image data.

    /** Alloc new image data of given dimensions and format.
     * Will use pfn_avpicture_alloc() for picture creation,
     *  thus the clear() will call pfn_avpicture_free(). */
    bool create(MSize dim, /*(AVPixelFormat)*/int avpic_fmt);

    /** Take ownership of the frame data and/or frame pointer.
     *  The frame DATA and the structure itself WILL BE DELETED in destructor.
     *
     *  @param to_be_deleted: if TRUE the provided pointer to  (AVFrame) object
     *  shall be treated as heap-allocation and deleted in destructor via
     *  pfn_deallocate_frame(AVFrame**). */
    bool imbue(AVFrame* frame);

    /** Take ownership of the picture data.
     *  The picture's DATA WILL BE DELETED in destructor. */
    bool imbue(const AVPicture& pic,
               int format = -1,
               MSize dim = MSize(-1, -1));

    AVPicture unsafe_access() const;
    int fmt() const;

    int w() const;
    int h() const;
    MSize dimensions() const;

private:
    class MImagePV;
    std::shared_ptr<MImagePV> pv;
};

bool operator < (const ZMB::MRegion& lhs, const ZMB::MRegion& rhs);

}//ZMB
#endif // MIMAGE_H
