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


#include "sws_imgscaler.h"
#include "mbytearray.h"

extern "C" {
    #include <libswscale/swscale.h>
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <assert.h>
}

namespace ZMB {

class SwsImgScaler::impl
{
public:
    impl() : sws(nullptr)
    {

    }
    ~impl()
    {
        sws_freeContext(sws);
    }

    void check_create_sws(const ZMB::MSize& in, AVPixelFormat fmt_in,
                          const ZMB::MSize& out, AVPixelFormat out_fmt)
    {
        if (!out_image.empty() && out_image.fmt() == out_fmt
                && out_image.dimensions() == out)
        { return; }

        out_size = out;

        AVPicture pic; memset(&pic, 0, sizeof(pic));
        avpicture_alloc(&pic, out_fmt, out.width(), out.height());
        out_image.imbue(pic);

        if(nullptr != sws)
        {
            sws_freeContext(sws);
            sws = nullptr;
        }

        sws = sws_getContext(in.width(), in.height(), fmt_in,
                             out.width(), out.height(),
                             out_fmt,
                             SWS_FAST_BILINEAR, NULL, NULL, NULL);

    }

    struct SwsContext* sws;
    ZMB::MImage out_image;
    ZMB::MSize out_size;
};

SwsImgScaler::SwsImgScaler(): pv (std::make_shared<SwsImgScaler::impl>())
{
}

bool SwsImgScaler::isNull() const
{
   return pv->out_image.empty();
}

ZMB::MSize SwsImgScaler::dest_size() const
{
    return pv->out_image.dimensions();
}

ZMB::MImage SwsImgScaler::scale(const AVPicture* av_pic,
                                const ZMB::MSize& in_size,
                                AVPixelFormat av_fmt, const ZMB::MSize& output_size,
                                AVPixelFormat out_fmt)
{
    pv->check_create_sws(in_size, (AVPixelFormat)av_fmt, output_size, out_fmt);
    AVPicture p_out = pv->out_image.unsafe_access();

    sws_scale(pv->sws, av_pic->data, av_pic->linesize,
              0, in_size.height(), p_out.data, p_out.linesize);

    return pv->out_image;

}


ZMB::MImage SwsImgScaler::scale(const ZMB::MImage& input,
                                const ZMB::MSize& output_size, AVPixelFormat out_fmt)
{
    auto pic = input.unsafe_access();
    return scale(&pic, input.dimensions(), (AVPixelFormat)input.fmt(), output_size, out_fmt);
}


}//namespace ZMB
