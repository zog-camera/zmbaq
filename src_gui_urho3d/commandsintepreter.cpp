#include "commandsintepreter.h"
#include <Urho3D/IO/Log.h>
#include <Urho3D/Engine/Console.h>
#include "src/zmbaq_common/zmb_str.h"

namespace ZMGUI
{

ZM_DEF_STATIC_CONST_STRING(ci_quit, "quit")

CommandsInterpreter::CommandsInterpreter(Urho3D::Context* ctx) : Urho3D::Object(ctx)
{
}


CommandsInterpreter::~CommandsInterpreter()
{

}

void CommandsInterpreter::handleCMD(const Urho3D::String &input)
{
    LOGRAW(input);
    if (ZConstString(input.CString(), input.Length()) == ci_quit)
    {
        exit(0);
    }
}

}


