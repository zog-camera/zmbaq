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


#ifndef LOGSERVICE_H
#define LOGSERVICE_H

#include <string>
#include "zmbaq_common.h"
#include "mbytearray.h"
#include "genericstreamwriter.h"

//aligned by 8-characters string:
ZM_DEF_STATIC_CONST_STRING(g_logservice_all_tag, "all")

static const u_int8_t severity_strs_align = 8;
static const char severity_strs[] = "DEBUG\0\0\0INFO\0\0\0\0WARNING\0ERROR\0\0\0";


namespace logservice {

class LogService;

enum LogLevel{DEBUG, INFO, WARNING, ERROR};


void linit(const Json::Value* dest_config, LogLevel lvl = LogLevel::DEBUG);
void ldestroy();

//get default config:
Json::Value ldefaults();

void lwrite(const ZConstString& obj_name, const ZConstString& msg, LogLevel lvl = DEBUG);
void lwrite(const ZConstString& obj_name, const ZMBCommon::MByteArray& str, LogLevel lvl = DEBUG);
void lwrite(const ZConstString& obj_name, const Json::Value& obj, LogLevel lvl = DEBUG);
void lwrite(const ZConstString& obj_name, const ZMBCommon::MByteArrayList& msg, LogLevel lvl = DEBUG);
void lwrite(const ZConstString& obj_name, const std::string& str, LogLevel lvl = DEBUG);
void lwrite(const ZConstString& obj_name, SHP(ZMBCommon::MByteArray) str, LogLevel lvl = DEBUG);

//get current level:
LogLevel llevel();

/** Get the singleton pointer.
 *  You may also manually create such objects
 *  when logservice.impl.h included.*/
LogService* instance();

//force to use new log destination config:
LogService* instance(const Json::Value* dest_config);

}



#define LDEBUG(OBJ_NAME, MSG) logservice::lwrite((OBJ_NAME), (MSG), logservice::LogLevel::DEBUG)
#define LINFO(OBJ_NAME, MSG) logservice::lwrite((OBJ_NAME), (MSG), logservice::LogLevel::INFO)
#define LWARNING(OBJ_NAME, MSG) logservice::lwrite((OBJ_NAME), (MSG), logservice::LogLevel::WARNING)
#define LERROR(OBJ_NAME, MSG) logservice::lwrite((OBJ_NAME), (MSG), logservice::LogLevel::ERROR)

#define ADEBUG(MSG)  logservice::lwrite(g_logservice_all_tag, (MSG), logservice::LogLevel::DEBUG)
#define AINFO(MSG) logservice::lwrite(g_logservice_all_tag, (MSG), logservice::LogLevel::INFO)
#define AWARNING(MSG)  logservice::lwrite(g_logservice_all_tag, (MSG), logservice::LogLevel::WARNING)
#define AERROR(MSG)  logservice::lwrite(g_logservice_all_tag, (MSG), logservice::LogLevel::ERROR)




#endif // LOGSERVICE_H
