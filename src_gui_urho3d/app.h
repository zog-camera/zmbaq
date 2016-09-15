//
// Copyright (c) 2008-2015 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "Sample.h"
#include "commandsintepreter.h"
#include "console.h"

#include "zmbaq_common/zmbaq_common.h"
#include "../src_videoentity/survlobj.h"
#include <Poco/ThreadPool.h>

namespace Urho3D
{

class Node;
class Scene;
class Console;

}

namespace ZMGUI {


/// Render to texture example
/// This sample demonstrates:
///     - Creating two 3D scenes and rendering the other into a texture
///     - Creating rendertarget texture and material programmatically
class App : public Sample
{
    URHO3D_OBJECT(App, Sample);

public:
    /// Construct.
    App(Context* context);

    /// Setup after engine initialization and before running the main loop.
    virtual void Start();

private:
    void CreateUI();
    /// Construct the scene content.
    void CreateScene();
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Set up a viewport for displaying the scene.
    void SetupViewport();
    /// Subscribe to application-wide logic update events.
    void SubscribeToEvents();
    /// Read input and moves the camera.
    void MoveCamera(float timeStep);

    /// Handle the logic update event.
    void HandleUpdate(StringHash eventType, VariantMap& eventData);

        /// Handle user input either from the engine console or standard input.
    void HandleInput(const String& input);
     /// Handle console command event.
    void HandleConsoleCommand(StringHash eventType, VariantMap& eventData);
    /// Handle ESC key down event to quit the engine.
    void HandleKeyDown(StringHash eventType, VariantMap& eventData);

    /// Print text to the engine console and standard output.
    void Print(const String& output);

    SharedPtr<ZMGUI::Console> console;
    SharedPtr<ZMGUI::CommandsInterpreter> cmd_interpet;

    ZMBEntities::SurvlObj suo; //< video survl. object

};
}//nm ZMGUI

