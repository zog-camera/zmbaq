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


#ifndef IMGSCALER_H
#define IMGSCALER_H
#include <memory>
#include "mimage.h"
#include "zmbaq_common/zmbaq_common.h"
#include <functional>

extern "C" {
    #include <libavutil/pixfmt.h>
}


struct AVFrame;
struct AVPicture;

class MByteArray;

namespace ZMB {

/** Rescaler that is using fast sws_scale() methods.*/
class SwsImgScaler
{

public:
    std::function<void(SwsImgScaler* sc, ZMBCommon::ZConstString msg)>*
        pfn_sig_error() const;

    SwsImgScaler();

    bool isNull() const;

    ZMB::MSize dest_size() const;
    int dest_fmt() const;

    /** Any AV_PIX_FMT to ZMB::MImage:RGB888.
     * @param av_fmt: input image fmt (enum AVPixelFormat) casted to (int). */
    ZMB::MImage scale(const AVPicture* av_pic,
                      const ZMB::MSize& in_size,
                      AVPixelFormat av_fmt, const ZMB::MSize& output_size,
                      AVPixelFormat out_fmt = AV_PIX_FMT_RGB24);

    /** Rescale. Input iamge format must be one of these:
     * ZMB::MImage::Format_RGB888, ZMB::MImage::Format_RGB32,
     *  ZMB::MImage::Format_ARGB32, ZMB::MImage::Format_RGBA8888 */
    ZMB::MImage scale(const ZMB::MImage& input, const ZMB::MSize& output_size,
                      AVPixelFormat out_fmt = AV_PIX_FMT_RGB24);

private:
    class impl;
    std::shared_ptr<impl> pv;
};

}//namespace ZMB

#endif // IMGSCALER_H
