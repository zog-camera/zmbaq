#include "movement_detection_task.h"

#include "src/sws_imgscaler.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
}

#include <map>
#include "Poco/Timespan.h"
#include "Poco/Timestamp.h"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/background_segm.hpp>

#ifndef __LOCAL_JSON_EXT__
#define __LOCAL_JSON_EXT__ 1
#define JSON_EXTR_INT(JOBJECT, KEY, DEFAULT_VAL) \
    (JOBJECT).get((KEY), Json::Value((DEFAULT_VAL))).asInt()

#define JSON_EXTR_DBL(JOBJECT, KEY, DEFAULT_VAL) \
    (JOBJECT).get((KEY), Json::Value((DEFAULT_VAL))).asDouble()

#endif

#include <Poco/Timestamp.h>
#include "src/zmbaq_common/mbytearray.h"
#include <map>
#include <iostream>
#include "delaunay/Triangulation.h"

namespace CVBGS {

/** A motion detector's point of view on events.
 * That could be either nothing, a light blick, an object motion.
 * Current event state has an uncertain state(null), active states(Invoked, Moving),
 * calm state.
*/
struct MotionDescription
{
    enum Type   { Uncertain, Blick, Motion };
    enum State { Null, Invoked, Moving, Calmed };

    MotionDescription() : type(Uncertain), state(Null) {}
    MotionDescription(const Type &type, const State &state) : type(type), state(state) {}

    Type type;
    State state;
};

/** A pair of current timestamp and a time segment.
 * Timestamp is set to beginning of the Epoch in default constructor
 * and the time segment is 0.
*/
struct TimeSegment : public std::pair<Poco::Timespan/*segment legth*/,
                                    Poco::Timestamp /*current time or Epoch*/>
{
    TimeSegment()
    {
        clear();
    }

    TimeSegment(Poco::Timespan duration) : TimeSegment()
        { set_time_segment(duration); }

    void set_time_segment(Poco::Timespan duration)
        { first = duration; }

    inline const Poco::Timestamp& ts() const {return second;}
    inline const Poco::Timespan& duration() const {return first;}

    /*Set to current time.*/
    void time() { second.update(); }

    /** @return TRUE if is not in 0-state(Epoch begin)*/
    bool is_timed() const
    {
        return second > Poco::Timestamp::TIMEVAL_MIN;
    }
    bool is_elapsed() const
    {
        return second.isElapsed(first.totalMicroseconds());
    }

    /** reset to the beginning of the Epoch */
    void clear()
    {
        second = Poco::Timestamp::TIMEVAL_MIN;
        set_time_segment(0);
    }
};


//==============================
/** Sets low/high level with a time delay,
 * depending on current value.
 * (input tri-state){-1,0,1} --> (trigger)[state: {off,on}]
 * First input tristate does not modify the triggers state,
 * but it'll ignite the timer (presented as integer value).
 * On next input tristate, the trigger will change it's state or reset the delay timer.
 *
 * 1. If the current state of the trigger is "off"(0) and tri-state valus is 1:
 *
 * (a) if the delay is timed out then we set trigger's state to On(1)
 *  and reset the delay timer to 0-state.
 *
 * (b) if the delay is NOT timed out : nothing happens, wait for next input value.
 *
 * 2. Case the current state is "on"(1) and tri-state is 1 or 0:
 *    Clear the delay timer to 0-state, unused until changed value comes on input.
 *
 * 3. Case state == "on"(1) and input value == -1:
 * (a)if the delay is timed out then we set trigger's state to Off(0)
 *  and reset the delay timer.
 *
 * (b) if the delay is NOT timed out : nothing happens, wait for next input value.
 *
 * 4. Case state == "off"(0) and input value == -1:
 *  and reset the delay timer to 0-state.
 *
 *
 * @verbatim
 Suppose trigger's delay equals 2 time units presented as "-",
 an example of input and output levels is shown below.

Input tristate {-1,0,1}:
1       ___________
       /           \                     /\
0-----+------------+---------+----------+-+----
                   \________/
-1
Trigger's state:
On       ___________
        /           \
of-----t-+-----------t-------t----------t-t----

"t" stands for trigger's delay timer reset, as we can see only 1 of the input
tristate pulses changed the trigger's state and made it to keep level On
during some time segment.

 * @endverbatim
*/
struct TresholdDelayedTrigger
{
    enum State { Off = 0, On = 1 };

    TresholdDelayedTrigger(): state(Off) {  }

    TresholdDelayedTrigger(TimeSegment time_delay)
        : state(Off), delay(time_delay) {  }

    inline void invert()
        { state = (Off == state) ? On : Off; }

    /** First call does not change the trigger's state,
     *  but will time the delay, next call can return modified state of the trigger.*/
    State input(int tristate/*{-1, 0, 1}*/)
    {
        if ((state == Off && tristate > 0) ||
            (state == On  && tristate < 0))
        {   //case we have level change

            if (!delay.is_timed())
            { //set up the time segment if it was not and return(making a delay)
                delay.time();
                return state;
            }

            //we have a time out, let's invert the state
            if (delay.is_elapsed())
            {//change to low level after a timeout, okay, seem not to be a light blick
                invert();
                delay.clear();
            }
            //else do nothing, wait for next value on input

        }
        else /*if (0 == tristate
                 || 1 == tristate && this->state == 1
                 || 0 == tristate && this->state == 0)*/
            if(delay.is_timed())
            {//no level change, reset the delay timer to 0-state
                delay.clear();
            }

        return state;
    }

    void set_delay(const Poco::Timespan& delay_usec)
        { delay.set_time_segment(delay_usec);}


    TimeSegment delay;
    State     state;
};

//==============================
/** A structure that is able to track tristate{-1. 0, 1} changes that
 * describe frame's motion alert state.
 * "-1" stands for negative difference in motion sensor's value(a decrease of motion).
 * "0" value stands for no changes,
 * "1" stands for increase of moving objects.
*/
struct MotionDelayedTrigger
{
    MotionDelayedTrigger()
    {
        blick_trigger.set_delay(Poco::Timespan(100 * 1000/*usec*/));
        object_motion_trigger.set_delay(Poco::Timespan(500 * 1000/*usec*/));
    }

    /** @return a description of the event, it can be described as moving or as idle object*/
    MotionDescription track(int tristate)
    {
        auto st_blick = blick_trigger.input(tristate);
        auto st_object = object_motion_trigger.input(TresholdDelayedTrigger::On == st_blick? 1 : -1);
        MotionDescription::Type mo_type = MotionDescription::Type::Uncertain;
        MotionDescription::State mo_state = MotionDescription::State::Null;

        if (TresholdDelayedTrigger::On == st_blick)
            mo_type = MotionDescription::Type::Blick;

        if (TresholdDelayedTrigger::On == st_object)
            mo_type = MotionDescription::Type::Motion;

        if (TresholdDelayedTrigger::Off == st_object)
        {
            mo_state = (TresholdDelayedTrigger::On == st_blick?
                        MotionDescription::State::Invoked : MotionDescription::State::Null);
        }
        else
        {//On == st_object
            mo_state = (TresholdDelayedTrigger::On == st_blick?
                            MotionDescription::State::Moving : MotionDescription::State::Calmed);
        }
        return {mo_type, mo_state};
    }

    void set_blick_treshold_delay(const Poco::Int64& msec) {
        blick_trigger.set_delay(Poco::Timespan(msec * 1000));
    }

    void set_object_motion_time(const Poco::Int64& msec) {
        object_motion_trigger.set_delay(Poco::Timespan(msec * 1000));;
    }


    TresholdDelayedTrigger blick_trigger;
    TresholdDelayedTrigger object_motion_trigger;
};

//==============================

struct BGSParams
{
    BGSParams()
    {
        Json::Value empty;
        set_params(empty);
    }

    void set_params(const Json::Value& params)
    {
        with_downscale = 0 < JSON_EXTR_INT(params, "downscale", 1)? true : false;
        skip = JSON_EXTR_INT(params, "skip", 40);
        threshold = JSON_EXTR_DBL(params, "threshold", 0.15);
        deviation_ratio = JSON_EXTR_DBL(params, "deviation_ratio", 0.005);
        blur_sigma = JSON_EXTR_DBL(params, "blur_sigma", 2.0);
    }

    bool with_downscale;
    int skip;
    int frame_cnt;

    double threshold;
    double blur_sigma;
    double deviation_ratio;

};

/** Makes video frame motion detection based on MOG2 subtraction algo.*/
class MOG2Algo
{
public:
    MOG2Algo()
    {
        max_frame_size = ZMB::MSize(1920, 1080);
        mog2 = cv::createBackgroundSubtractorMOG2(500, 16, false);
        mog2->setVarMin(100);

        Element = cv::getStructuringElement( 0, cv::Size( 2, 2 ), cv::Point( -1, -1 ) );
    }

    // returns 'true' if there's movement.
    MotionDescription proc(const ZMB::MImage& frame)
    {
        //Determine the scaling factor for the downscale
        //Approximate number of pixels after rescale: full HD downscaled by 8X

        ZMB::MSize inp_sz = frame.dimensions();
        if (params.with_downscale)
        {
            int width   = inp_sz.width();
            int height  = inp_sz.height();

            //downscaling:
            float refNumPixels = max_frame_size.square() / 64.0f;
            float scale         = std::sqrt((float)(width * height)/refNumPixels);
            int down_w = std::round((float)width / scale);
            int down_h = std::round((float)height / scale);


            if (nullptr == scaler)
            {
               scaler = std::make_shared<ZMB::SwsImgScaler>();
            }
            img = scaler->scale(frame, ZMB::MSize(down_w,down_h));
        }
        else
        {
            img = frame;
        }
        AVPicture avp = img.unsafe_access();
        auto sz = img.dimensions();
        resized = cv::Mat(sz.height(), sz.width(), CV_8UC3, (void*)avp.data[0]);

        mog2->apply(resized, mask);

        MotionDescription res;
        //BGS needs a warm-up
        if (++params.frame_cnt < params.skip)
            return res;

        mask /= 255;

        cv::erode(mask, mask, Element, cv::Point(-1, -1), 1);

        converted = cv::Mat(sz.height(), sz.width(),CV_32FC1);
        mask.convertTo(converted,CV_32FC1);

        blurred = cv::Mat(sz.height(), sz.width(),CV_32FC1);
        cv::GaussianBlur(converted,blurred,cv::Size(0,0), params.blur_sigma);

        thresholded = cv::Mat(sz.height(), sz.width(), CV_32FC1);
        cv::threshold(blurred, thresholded, params.threshold,
                      1.0, cv::THRESH_BINARY);

        Poco::Int64 nonzero = cv::countNonZero(thresholded);
        Poco::Int64 level = params.deviation_ratio * sz.height() * sz.width();

        res = frame_treshold_track.track(fn_level_tristate(nonzero, level));
        return res;
    }
    inline int fn_level_tristate(const Poco::Int64& value, const Poco::Int64& level)
    {
        return (value > level)? 1 : (value < level? -1 : 0);
    }

    ZMB::MSize max_frame_size;
    BGSParams params;//< you may modify params before valling proc() method;

private:
    cv::Mat Element;
    cv::Mat converted;
    cv::Mat blurred;
    cv::Mat thresholded;
    cv::Mat mask;
    ZMB::MImage img;
    cv::Mat resized;

    //for tracking of the whole frame:
    MotionDelayedTrigger frame_treshold_track;

    std::shared_ptr<ZMB::SwsImgScaler> scaler;
    cv::Ptr <cv::BackgroundSubtractorMOG2> mog2;
};

}//namespace CVBGS

namespace ZMBEntities {

using namespace ZMBCommon;

class MovementDetector
{
public:
    enum DetectionMode{FULL_FRAME,
                       POLY_IGNORE_ZONES,
                       POLY_INTEREST_ZONES,
                       RECTANGLE_INTEREST_ZONE};

    MovementDetector() : mode(FULL_FRAME)
    {
        need_mask_update = true;
    }
    void clear()
    {
        need_mask_update = true;
        mode = DetectionMode::FULL_FRAME;
        rect_zones_map.clear();
        ignored_polygonal_zones_map.clear();
        interest_polygonal_zones_map.clear();
    }

    bool add_rectangular_enabled_zones(const ZMB::MRegion* rectangles_vector, int len)
    {
        auto rvend = rectangles_vector + len;
        for (auto iter = rectangles_vector; iter != rvend; ++iter)
        {
            const ZMB::MRegion& r(*iter);
            std::shared_ptr<CVBGS::MOG2Algo> mg = std::make_shared<CVBGS::MOG2Algo>();
            if (r.square() < 128 * 128)
            {// disable downscaling for little blocks
               mg->params.with_downscale = false;
            }
            rect_zones_map[r] = mg;
        }
        mode = DetectionMode::RECTANGLE_INTEREST_ZONE;
    }

    bool add_polygonal_zone(const glm::ivec2* img_coord_polyline,
                            int len,
                            ZConstString zone_name)
    {
        std::cerr << "NOT IMPLEMENTED\n";
        mode = DetectionMode::FULL_FRAME;
    }

    typedef std::map<ZMBCommon::MByteArray, std::vector<glm::ivec2>> LinesMap;
    typedef std::pair<ZMBCommon::MByteArray, std::vector<glm::ivec2>> NamedLine;

    void detect(const ZMB::MImage& frame)
    {
        if (need_mask_update)
        {
            std::vector<JVa::Vector3> input;
            const LinesMap& zmap(ignored_polygonal_zones_map);
            //for all polygonal convex zones
            for (const NamedLine& line : zmap)
            {
                //vec2(x,y)->vec3(x,y,0)
                for (const glm::ivec2& v : line.second)
                {
                    input.push_back(JVa::Vector3(v.x, v.y, 0.0));
                }
                //convex hull -> triangles via Delaunay algorithm
                std::shared_ptr<JVa::Result> tri_res = JVa::triangulate(input.data(), input.size());
                if (tri_res->n > 0)
                {
                    //for all faces (triangles) of the area inside the contour:
                    for (int c = 0; c < tri_res->n; ++c)
                    {
                        if (0 < tri_res->faces[c].idx)
                            continue;//skip non-existing elements
                        //compute the bounding xy-rectangle:
//                        ZMB::MRegion r;
//                        r.top_left().x;


                    }

                }
            }
//            JVa::triangulate();

        }
        CVBGS::MotionDescription desc;
        switch (mode) {
        case DetectionMode::FULL_FRAME:
            if (nullptr == full_frame_mog2)
                full_frame_mog2 = std::make_shared<CVBGS::MOG2Algo>();
            desc = full_frame_mog2->proc(frame);
            break;
        case DetectionMode::POLY_IGNORE_ZONES:
            break;
        case DetectionMode::POLY_INTEREST_ZONES:
            break;
        case DetectionMode::RECTANGLE_INTEREST_ZONE:

            for (auto desc: rect_zones_map)
            {
//                if (desc.second- == CVBGS::MotionDescription::Moving)
//                    break;
            }
            break;
        default:
            break;
        }
    }

    DetectionMode mode;
    std::shared_ptr<CVBGS::MOG2Algo> full_frame_mog2;
    std::map<ZMB::MRegion, std::shared_ptr<CVBGS::MOG2Algo>> rect_zones_map;

    std::map<ZMBCommon::MByteArray/*name*/, std::vector<glm::ivec2>/*convex hull*/>
        ignored_polygonal_zones_map,
        interest_polygonal_zones_map;
    cv::Mat enabled_detection_mask;
    bool need_mask_update;

};

MovementDetectionTask::MovementDetectionTask(const std::string &name)
    : Poco::Task(name)
{

}

} //namespace ZMBEntities

