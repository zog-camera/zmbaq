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

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/RenderSurface.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/IO/IOEvents.h>
#include <Urho3D/Engine/Console.h>
#include <Urho3D/Engine/EngineEvents.h>
#include <Urho3D/IO/FileSystem.h>

#include "app.h"

#include <Urho3D/DebugNew.h>

URHO3D_DEFINE_APPLICATION_MAIN(ZMGUI::App)

#include "json/reader.h"


namespace ZMGUI {

App::App(Context* context) :
    Sample(context)
{
    console = nullptr;
}

void App::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    CreateUI();

    // Create the UI content
    CreateInstructions();

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update events
    SubscribeToEvents();

    const Vector<Urho3D::String>& args = Urho3D::GetArguments();
    {//read JSON-settings:
        Urho3D::String ext_json(".json");
        Json::Value cfg;
        for (auto iter = args.Begin(); iter != args.End(); ++iter)
        {
            const Urho3D::String& arg_str = (*iter);
            if (arg_str.Contains(ext_json))
            {
              //TODO: fix
            }
        }
//        AERROR(std::string("No input config file provided.\n"));
    }
}

void App::CreateUI()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    UI* ui = GetSubsystem<UI>();

     // Set up global UI style into the root UI element
    XMLFile* style = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
    ui->GetRoot()->SetDefaultStyle(style);


    // Show the console by default, make it large. Console will show the text edit field when there is at least one
    // subscriber for the console command event
    console = new ZMGUI::Console(context_);
    console->SetNumRows(GetSubsystem<Graphics>()->GetHeight() / 16);
    console->SetNumBufferedRows(2 * console->GetNumRows());
    console->SetCommandInterpreter(GetTypeName());
    console->SetVisible(true);
    console->SetFocusOnShow(true);
    console->GetCloseButton()->SetVisible(true);
    console->SetDefaultStyle(style);

    FileSystem* fs = GetSubsystem<FileSystem>();
    fs->UnsubscribeFromEvent(E_CONSOLECOMMAND);

    cmd_interpet = new ZMGUI::CommandsInterpreter(context_);

    // Show OS mouse cursor
    GetSubsystem<Input>()->SetMouseVisible(true);


    // Create a Cursor UI element because we want to be able to hide and show it at will. When hidden, the mouse cursor will
    // control the camera, and when visible, it will interact with the UI
    SharedPtr<Cursor> cursor(new Cursor(context_));
    cursor->SetStyleAuto();
    ui->SetCursor(cursor);
    // Set starting position of the cursor at the rendering window center
    Graphics* graphics = GetSubsystem<Graphics>();
    cursor->SetPosition(graphics->GetWidth() / 2, graphics->GetHeight() / 2);

//    // Load UI content prepared in the editor and add to the UI hierarchy
//    SharedPtr<UIElement> layoutRoot = ui->LoadLayout(cache->GetResource<XMLFile>("UI/UILoadExample.xml"));
//    ui->GetRoot()->AddChild(layoutRoot);
}

void App::CreateScene()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

}

void App::CreateInstructions()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    UI* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    Text* instructionText = ui->GetRoot()->CreateChild<Text>();
    instructionText->SetText("Use WASD keys and mouse/touch to move");
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
}

void App::SetupViewport()
{
//    Renderer* renderer = GetSubsystem<Renderer>();
}

void App::MoveCamera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (GetSubsystem<UI>()->GetFocusElement())
        return;

    Input* input = GetSubsystem<Input>();

    // Movement speed as world units per second
    const float MOVE_SPEED = 20.0f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    IntVector2 mouseMove = input->GetMouseMove();
    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input->GetKeyDown('W'))
    {
        //do some view translation here
    }
}

void App::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(App, HandleUpdate));

    // Subscribe to console commands and the frame update
    SubscribeToEvent(E_CONSOLECOMMAND, URHO3D_HANDLER(App, HandleConsoleCommand));

    // Subscribe key down event
    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(App, HandleKeyDown));

    // Hide logo to make room for the console
    SetLogoVisible(false);

    // Show OS mouse cursor
    GetSubsystem<Input>()->SetMouseVisible(true);

    // Open the operating system console window (for stdin / stdout) if not open yet
    OpenConsoleWindow();

    // Randomize from system clock
    SetRandomSeed(Time::GetSystemTime());

}

void App::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);
}

void App::HandleConsoleCommand(StringHash eventType, VariantMap& eventData)
{
    using namespace ConsoleCommand;
    auto pid_str = eventData[P_ID].GetString();
    if (GetTypeName() == pid_str)
    {
        HandleInput(eventData[P_COMMAND].GetString());
        SendEvent(E_LOGMESSAGE, eventData);
    }
}

void App::HandleInput(const String &input)
{
    cmd_interpet->handleCMD(input);
}

void App::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
    // Unlike the other samples, exiting the engine when ESC is pressed instead of just closing the console
    int kint = eventData[KeyDown::P_KEY].GetInt();
    switch (kint) {
    case KEY_ESCAPE:
        console->Toggle();
        break;
//    case (int)'~':
//    case (int)'`':
//        console->Toggle();
    default:
        break;
    }
}

void App::Print(const String& output)
{

}

}//ZMGUI
