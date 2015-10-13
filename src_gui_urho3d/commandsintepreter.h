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
    OBJECT(ZMGUI::CommandsInterpreter)
    CommandsInterpreter(Urho3D::Context* ctx);
    virtual ~CommandsInterpreter();

    void handleCMD(const Urho3D::String& input);

};

}

#endif // COMMANDSINTEPRETER_H
