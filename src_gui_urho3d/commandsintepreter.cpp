#include "commandsintepreter.h"
#include <Urho3D/IO/Log.h>
#include <Urho3D/Engine/Console.h>


namespace ZMGUI
{

CommandsInterpreter::CommandsInterpreter(Urho3D::Context* ctx) : Urho3D::Object(ctx)
{
}


CommandsInterpreter::~CommandsInterpreter()
{

}

void CommandsInterpreter::handleCMD(const Urho3D::String &input)
{
    URHO3D_LOGRAW(input);
    if (input == "quit")
    {
        exit(0);
    }
}

}


