#ifndef FFILEWRITER_PV_INL_HPP
#define FFILEWRITER_PV_INL_HPP

namespace ZMB {

static std::string ts_to_string(int64_t ts)
{
  size_t size = 0;
  std::string buffer;
  buffer.resize(AV_TS_MAX_STRING_SIZE, '\0');

  if (ts == int64_t(AV_NOPTS_VALUE))
    size = snprintf(&buffer[0], AV_TS_MAX_STRING_SIZE, "NOPTS");
  else
    size = snprintf(&buffer[0], AV_TS_MAX_STRING_SIZE, "%" PRIu64, ts);
  return buffer.substr(0, size);
}

static std::string ts_to_timestring(int64_t ts, AVRational *tb)
{
  size_t size = 0;
  std::string buffer;
  buffer.resize(AV_TS_MAX_STRING_SIZE, 0x00);

  if (ts == int64_t(AV_NOPTS_VALUE))
    size = snprintf(&buffer[0], AV_TS_MAX_STRING_SIZE, "NOPTS");
  else
    size = snprintf(&buffer[0], AV_TS_MAX_STRING_SIZE, "%.6g", av_q2d(*tb) * ts);
  return buffer.substr(0, size);
}

static std::string packet_to_string(const std::string& tag, const AVFormatContext *av_fmt_ctx, const AVPacket *av_pkt)
{
  AVRational *time_base = &av_fmt_ctx->streams[av_pkt->stream_index]->time_base;

  std::string buffer;
  buffer.resize(255, 0x00);
  size_t len = snprintf(&buffer[0], buffer.size(),
                        "%s: pts: %s pts_time: %s dts: %s dts_time: %s duration: %s duration_time: %s stream_index: %d\n",
                        tag.c_str(),
                        ts_to_string(av_pkt->pts).c_str(), ts_to_timestring(av_pkt->pts, time_base).c_str(),
                        ts_to_string(av_pkt->dts).c_str(), ts_to_timestring(av_pkt->dts, time_base).c_str(),
                        ts_to_string(av_pkt->duration).c_str(), ts_to_timestring(av_pkt->duration, time_base).c_str(),
                        av_pkt->stream_index);

  return buffer.substr(0, len);
}

static std::string error_to_string(int errnum)
{
  std::string buff;
  buff.resize(1024, 0x00);

  if (av_strerror(errnum, &buff[0], buff.size()) == 0)
  {
    buff = buff.substr(0, buff.find_last_not_of(char(0)) + 1);
    return buff;
  }

  return std::string(std::to_string(errnum));
}

template<class TFunctorOnError>
struct FFileWriterPV
{
  
  AVFormatContext *outFmtContext;
  const AVFormatContext *inFmtContext;
  unsigned frame_cnt;

  int64_t pts_shift, dts_shift;

  TFunctorOnError onError;
  std::atomic_uint pb_file_size;
  std::string url;

  explicit FFileWriterPV(const AVFormatContext *av_in_fmt_ctx, const std::string& dst);
  ~FFileWriterPV();
  
  bool open(const std::string&);
  void close();
  void write(std::shared_ptr<av::Packet> input_avpacket);
};

template<class TFunctorOnError>
FFileWriterPV<TFunctorOnError>::FFileWriterPV(const AVFormatContext *av_in_fmt_ctx, const std::string& dst)
  :
  outFmtContext(nullptr),
  inFmtContext(av_in_fmt_ctx),
  frame_cnt(0)
{
  url = dst;
  pts_shift = 0;
  dts_shift = 0;
  pb_file_size.store(0u);
  assert(nullptr != av_in_fmt_ctx);

  try
  {
    open(dst);
  }
  catch(const std::exception& e)
  {
    close();
  }

}

template<class TFunctorOnError>
FFileWriterPV<TFunctorOnError>::~FFileWriterPV()
{
  close();
}

template<class TFunctorOnError>
bool FFileWriterPV<TFunctorOnError>::open(const std::string& dst)
{
  int errnum = 0;

  AVOutputFormat *av_out_fmt = av_guess_format(nullptr, /*filename*/dst.c_str(), nullptr);
  if (!av_out_fmt)
  {
    std::string msg("Can't guess format by passed dest: ");
    msg += dst;
    onError(msg);
    return false;
  }

  outFmtContext = avformat_alloc_context();
  outFmtContext->oformat = av_out_fmt;

  if (!outFmtContext)
  {
    onError("Could not create output context");
    return false;
  }

  for (uint32_t i = 0; i < inFmtContext->nb_streams; ++i)
  {
    if (inFmtContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
	{
	  AVStream *in_stream = inFmtContext->streams[i];
	  AVStream *out_stream = avformat_new_stream(outFmtContext, avcodec_find_encoder(in_stream->codec->codec_id));

	  if (!out_stream)
      {
        onError("Failed allocating output stream");
        goto _on_fail_;
      }

	  out_stream->time_base           = in_stream->time_base;
	  out_stream->codec->codec_id     = in_stream->codec->codec_id;
	  out_stream->codec->sample_aspect_ratio = in_stream->codec->sample_aspect_ratio;
	}
  }

  if (!(av_out_fmt->flags & AVFMT_NOFILE))
  {
    if (avio_open(&outFmtContext->pb, dst.c_str(), AVIO_FLAG_WRITE) < 0)
	{
	  std::string msg("Could not open output: ");
	  msg += dst;
	  onError(msg);
	  goto _on_fail_;
	}
  }

  errnum = avformat_write_header(outFmtContext, nullptr);
  if(0 != errnum)
    return false;

#ifndef NDEBUG
  av_dump_format(outFmtContext, 0, dst.c_str(), 1);
#endif
  return true;

_on_fail_:
  avformat_free_context(outFmtContext);
  return false;

}

template<class TFunctorOnError>
void FFileWriterPV<TFunctorOnError>::close()
{
  if (nullptr != outFmtContext)
  {
    if (!outFmtContext->pb->error && frame_cnt > 0)
	{
	  av_write_trailer(outFmtContext);
	}

    if ((outFmtContext->pb && !(outFmtContext->oformat->flags & AVFMT_NOFILE)))
	{
	  avio_closep(&outFmtContext->pb);
	}            

    avformat_free_context(outFmtContext);
    outFmtContext = nullptr;
  }
}

template<class TFunctorOnError>
void FFileWriterPV<TFunctorOnError>::write(std::shared_ptr<av::Packet> input_avpacket)
{
  assert (nullptr != inFmtContext);

  if (nullptr == outFmtContext)
  {
    bool ok = false;
    try {
      ok = open(url);
    }
    catch(const std::exception& e) {
      close();
    }
    assert(ok);
    if (!ok) return;
  }
  int av_stream_idx = input_avpacket->streamIndex();

  AVStream *in_stream  = inFmtContext->streams[av_stream_idx];
  AVStream *out_stream = outFmtContext->streams[0];


  if (0 == pts_shift)
  {
    if (input_avpacket->pts().timestamp() != int64_t(AV_NOPTS_VALUE))
	{
	  pts_shift = av_rescale_q_rnd(input_avpacket->pts().timestamp(), in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF);
	  dts_shift = av_rescale_q_rnd(input_avpacket->dts().timestamp(), in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF);
	}
  }

  //        int orig_idx = input_avpacket->streamIndex();
  //        int64_t orig_pos = input_avpacket->pos;

  std::error_code _e;
  AVPacket opkt = input_avpacket->makeRef(_e);
  opkt.stream_index = out_stream->index;
  opkt.pts = input_avpacket->pts().timestamp()  == int64_t(AV_NOPTS_VALUE) ? AV_NOPTS_VALUE
    : av_rescale_q_rnd(input_avpacket->pts().timestamp(), in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF);
  opkt.dts = av_rescale_q_rnd(input_avpacket->dts().timestamp(), in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF);

  opkt.duration = av_rescale_q(input_avpacket->duration(), in_stream->time_base, out_stream->time_base);
  opkt.pts -= pts_shift;
  opkt.dts -= dts_shift;
  opkt.pos = -1;

  //#ifndef NDEBUG
  //        auto mb_log = packet_to_string("in", av_in_fmt_ctx,
  //                                       const_cast<AVPacket*>(input_avpacket));
  //        LDEBUG(std::string("avpacket"), mb_log.get_const_str());
  //#endif

  /** file size indication. Note: always access packet data before *_write_frame()
   *  functions are called, they purge the packet's payload when done.*/
  pb_file_size.fetch_add(opkt.size);

  av_interleaved_write_frame(outFmtContext, &opkt);

  //        //restore some values:
  //        input_avpacket->pts += pts_shift;
  //        input_avpacket->dts += dts_shift;
  //        input_avpacket->pos = orig_pos;
  //        input_avpacket->stream_index = orig_idx;

  ++(frame_cnt);
}

}//namespace ZMB

#endif //FFILEWRITER_PV_INL_HPP
