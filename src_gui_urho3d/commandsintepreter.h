#ifndef COMMANDSINTEPRETER_H
#define COMMANDSINTEPRETER_H

#include <Urho3D/Core/Object.h>

#include <Urho3D/Core/Context.h>
//namespace Urho3D {
//        class Context;
//}

namespace ZMGUI
{

class CommandsInterpreter : public Urho3D::Object
{
public:
    URHO3D_OBJECT(ZMGUI::CommandsInterpreter, Urho3D::Object)
    CommandsInterpreter(Urho3D::Context* ctx);
    virtual ~CommandsInterpreter();

    void handleCMD(const Urho3D::String& input);

};

}

#endif // COMMANDSINTEPRETER_H
