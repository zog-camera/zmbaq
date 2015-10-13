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
#include "console.h"
#include <Urho3D/Container/Swap.h>

#include "Urho3D/Precompiled.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Core/CoreEvents.h"
#include "Urho3D/Engine/Console.h"
#include "Urho3D/Engine/EngineEvents.h"
#include "Urho3D/Graphics/Graphics.h"
#include "Urho3D/Graphics/GraphicsEvents.h"
#include "Urho3D/Input/Input.h"
#include "Urho3D/IO/IOEvents.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/UI/DropDownList.h"
#include "Urho3D/UI/Font.h"
#include "Urho3D/UI/LineEdit.h"
#include "Urho3D/UI/ListView.h"
#include "Urho3D/UI/ScrollBar.h"
#include "Urho3D/UI/Text.h"
#include "Urho3D/UI/UI.h"
#include "Urho3D/UI/UIEvents.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D {

void Swap(Urho3D::String& first, Urho3D::String& second)
{
   Urho3D::String temp = first;
   first = second;
   second = temp;
}

}

namespace ZMGUI
{

static const int DEFAULT_CONSOLE_ROWS = 16;
static const int DEFAULT_HISTORY_SIZE = 16;

Console::Console(Urho3D::Context* context) :
    Urho3D::Object(context),
    autoVisibleOnError_(false),
    historyRows_(DEFAULT_HISTORY_SIZE),
    historyPosition_(0),
    printing_(false)
{
    Urho3D::UI* ui = GetSubsystem<Urho3D::UI>();
    Urho3D::UIElement* uiRoot = ui->GetRoot();

    // By default prevent the automatic showing of the screen keyboard
    focusOnShow_ = !ui->GetUseScreenKeyboard();

    background_ = uiRoot->CreateChild<Urho3D::BorderImage>();
    background_->SetBringToBack(false);
    background_->SetClipChildren(true);
    background_->SetEnabled(true);
    background_->SetVisible(false); // Hide by default
    background_->SetPriority(200); // Show on top of the debug HUD
    background_->SetBringToBack(false);
    background_->SetLayout(Urho3D::LM_VERTICAL);

    rowContainer_ = background_->CreateChild<Urho3D::ListView>();
    rowContainer_->SetHighlightMode(Urho3D::HM_ALWAYS);
    rowContainer_->SetMultiselect(true);

    commandLine_ = background_->CreateChild<Urho3D::UIElement>();
    commandLine_->SetLayoutMode(Urho3D::LM_HORIZONTAL);
    commandLine_->SetLayoutSpacing(1);
    interpreters_ = commandLine_->CreateChild<Urho3D::DropDownList>();
    lineEdit_ = commandLine_->CreateChild<Urho3D::LineEdit>();
    lineEdit_->SetFocusMode(Urho3D::FM_FOCUSABLE);  // Do not allow defocus with ESC

    closeButton_ = uiRoot->CreateChild<Urho3D::Button>();
    closeButton_->SetVisible(false);
    closeButton_->SetPriority(background_->GetPriority() + 1);  // Show on top of console's background
    closeButton_->SetBringToBack(false);

    SetNumRows(DEFAULT_CONSOLE_ROWS);

    using namespace Urho3D;
    SubscribeToEvent(interpreters_, E_ITEMSELECTED, HANDLER(Console, HandleInterpreterSelected));
    SubscribeToEvent(lineEdit_, E_TEXTFINISHED, HANDLER(Console, HandleTextFinished));
    SubscribeToEvent(lineEdit_, E_UNHANDLEDKEY, HANDLER(Console, HandleLineEditKey));
    SubscribeToEvent(closeButton_, E_RELEASED, HANDLER(Console, HandleCloseButtonPressed));
    SubscribeToEvent(E_SCREENMODE, HANDLER(Console, HandleScreenMode));
    SubscribeToEvent(E_LOGMESSAGE, HANDLER(Console, HandleLogMessage));
    SubscribeToEvent(E_POSTUPDATE, HANDLER(Console, HandlePostUpdate));
}

Console::~Console()
{
    background_->Remove();
    closeButton_->Remove();
}

void Console::SetDefaultStyle(Urho3D::XMLFile* style)
{
    if (!style)
        return;

    background_->SetDefaultStyle(style);
    background_->SetStyle("ConsoleBackground");
    rowContainer_->SetStyleAuto();
    for (unsigned i = 0; i < rowContainer_->GetNumItems(); ++i)
        rowContainer_->GetItem(i)->SetStyle("ConsoleText");
    interpreters_->SetStyleAuto();
    for (unsigned i = 0; i < interpreters_->GetNumItems(); ++i)
        interpreters_->GetItem(i)->SetStyle("ConsoleText");
    lineEdit_->SetStyle("ConsoleLineEdit");

    closeButton_->SetDefaultStyle(style);
    closeButton_->SetStyle("CloseButton");

    UpdateElements();
}

void Console::SetVisible(bool enable)
{
    Urho3D::Input* input = GetSubsystem<Urho3D::Input>();
    background_->SetVisible(enable);
    closeButton_->SetVisible(enable);
    if (enable)
    {
        // Check if we have receivers for E_CONSOLECOMMAND every time here in case the handler is being added later dynamically
        bool hasInterpreter = PopulateInterpreter();
        commandLine_->SetVisible(hasInterpreter);
        if (hasInterpreter && focusOnShow_)
            GetSubsystem<Urho3D::UI>()->SetFocusElement(lineEdit_);

        // Ensure the background has no empty space when shown without the lineedit
        background_->SetHeight(background_->GetMinHeight());

        // Show OS mouse
        input->SetMouseVisible(true, true);
    }
    else
    {
        rowContainer_->SetFocus(false);
        interpreters_->SetFocus(false);
        lineEdit_->SetFocus(false);

        // Restore OS mouse visibility
        input->ResetMouseVisible();
    }
}

void Console::Toggle()
{
    SetVisible(!IsVisible());
}

void Console::SetNumBufferedRows(unsigned rows)
{
    if (rows < displayedRows_)
        return;

    rowContainer_->DisableLayoutUpdate();

    int delta = rowContainer_->GetNumItems() - rows;
    if (delta > 0)
    {
        // We have more, remove oldest rows first
        for (int i = 0; i < delta; ++i)
            rowContainer_->RemoveItem((unsigned)0);
    }
    else
    {
        // We have less, add more rows at the top
        for (int i = 0; i > delta; --i)
        {
            Urho3D::Text* text = new Urho3D::Text(context_);
            // If style is already set, apply here to ensure proper height of the console when
            // amount of rows is changed
            if (background_->GetDefaultStyle())
                text->SetStyle("ConsoleText");
            rowContainer_->InsertItem(0, text);
        }
    }

    rowContainer_->EnsureItemVisibility(rowContainer_->GetItem(rowContainer_->GetNumItems() - 1));
    rowContainer_->EnableLayoutUpdate();
    rowContainer_->UpdateLayout();

    UpdateElements();
}

void Console::SetNumRows(unsigned rows)
{
    if (!rows)
        return;

    displayedRows_ = rows;
    if (GetNumBufferedRows() < rows)
        SetNumBufferedRows(rows);

    UpdateElements();
}

void Console::SetNumHistoryRows(unsigned rows)
{
    historyRows_ = rows;
    if (history_.Size() > rows)
        history_.Resize(rows);
    if (historyPosition_ > rows)
        historyPosition_ = rows;
}

void Console::SetFocusOnShow(bool enable)
{
    focusOnShow_ = enable;
}

void Console::UpdateElements()
{
    int width = GetSubsystem<Urho3D::Graphics>()->GetWidth();
    const Urho3D::IntRect& border = background_->GetLayoutBorder();
    const Urho3D::IntRect& panelBorder = rowContainer_->GetScrollPanel()->GetClipBorder();
    rowContainer_->SetFixedWidth(width - border.left_ - border.right_);
    rowContainer_->SetFixedHeight(
        displayedRows_ * rowContainer_->GetItem((unsigned)0)->GetHeight() + panelBorder.top_ + panelBorder.bottom_ +
        (rowContainer_->GetHorizontalScrollBar()->IsVisible() ? rowContainer_->GetHorizontalScrollBar()->GetHeight() : 0));
    background_->SetFixedWidth(width);
    background_->SetHeight(background_->GetMinHeight());
}

Urho3D::XMLFile* Console::GetDefaultStyle() const
{
    return background_->GetDefaultStyle(false);
}

bool Console::IsVisible() const
{
    return background_ && background_->IsVisible();
}

unsigned Console::GetNumBufferedRows() const
{
    return rowContainer_->GetNumItems();
}

void Console::CopySelectedRows() const
{
    rowContainer_->CopySelectedItemsToClipboard();
}

const Urho3D::String& Console::GetHistoryRow(unsigned index) const
{
    return index < history_.Size() ? history_[index] : Urho3D::String::EMPTY;
}


bool Console::PopulateInterpreter()
{
    interpreters_->RemoveAllItems();

    Urho3D::HashSet<Object*>* receivers = context_->GetEventReceivers(Urho3D::E_CONSOLECOMMAND);
    if (!receivers || receivers->Empty())
        return false;

    Urho3D::Vector<Urho3D::String> names;
    for (auto iter = receivers->Begin(); iter != receivers->End(); ++iter)
        names.Push((*iter)->GetTypeName());
    Urho3D::Sort(names.Begin(), names.End());

    unsigned selection = Urho3D::M_MAX_UNSIGNED;
    for (unsigned i = 0; i < names.Size(); ++i)
    {
        const Urho3D::String& name = names[i];
        if (name == commandInterpreter_)
            selection = i;
        Urho3D::Text* text = new Urho3D::Text(context_);
        text->SetStyle("ConsoleText");
        text->SetText(name);
        interpreters_->AddItem(text);
    }

    const Urho3D::IntRect& border = interpreters_->GetPopup()->GetLayoutBorder();
    interpreters_->SetMaxWidth(interpreters_->GetListView()->GetContentElement()->GetWidth() + border.left_ + border.right_);
    bool enabled = interpreters_->GetNumItems() > 1;
    interpreters_->SetEnabled(enabled);
    interpreters_->SetFocusMode(enabled ? Urho3D::FM_FOCUSABLE_DEFOCUSABLE : Urho3D::FM_NOTFOCUSABLE);

    if (selection == Urho3D::M_MAX_UNSIGNED)
    {
        selection = 0;
        commandInterpreter_ = names[selection];
    }
    interpreters_->SetSelection(selection);

    return true;
}

void Console::HandleInterpreterSelected(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData)
{
    commandInterpreter_ = static_cast<Urho3D::Text*>(interpreters_->GetSelectedItem())->GetText();
    lineEdit_->SetFocus(true);
}

void Console::HandleTextFinished(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData)
{
    using namespace Urho3D::TextFinished;

    Urho3D::String line = lineEdit_->GetText();
    if (!line.Empty())
    {
        // Send the command as an event for script subsystem
        using namespace Urho3D::ConsoleCommand;

        Urho3D::VariantMap& newEventData = GetEventDataMap();
        newEventData[P_COMMAND] = line;
        newEventData[P_ID] = static_cast<Urho3D::Text*>(interpreters_->GetSelectedItem())->GetText();
        SendEvent(Urho3D::E_CONSOLECOMMAND, newEventData);

        // Store to history, then clear the lineedit
        history_.Push(line);
        if (history_.Size() > historyRows_)
            history_.Erase(history_.Begin());
        historyPosition_ = history_.Size();

        currentRow_.Clear();
        lineEdit_->SetText(currentRow_);
    }
}

void Console::HandleLineEditKey(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData)
{
    if (!historyRows_)
        return;

    using namespace Urho3D;
    using namespace UnhandledKey;

    bool changed = false;

    switch (eventData[P_KEY].GetInt())
    {
    case KEY_UP:
        if (historyPosition_ > 0)
        {
            if (historyPosition_ == history_.Size())
                currentRow_ = lineEdit_->GetText();
            --historyPosition_;
            changed = true;
        }
        break;

    case KEY_DOWN:
        if (historyPosition_ < history_.Size())
        {
            ++historyPosition_;
            changed = true;
        }
        break;

    default: break;
    }

    if (changed)
    {
        if (historyPosition_ < history_.Size())
            lineEdit_->SetText(history_[historyPosition_]);
        else
            lineEdit_->SetText(currentRow_);
    }
}

void Console::HandleCloseButtonPressed(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData)
{
    SetVisible(false);
}

void Console::HandleScreenMode(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData)
{
    UpdateElements();
}

void Console::HandleLogMessage(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData)
{
    // If printing a log message causes more messages to be logged (error accessing font), disregard them
    if (printing_)
        return;

    using namespace Urho3D::LogMessage;

    int level = eventData[P_LEVEL].GetInt();
    // The message may be multi-line, so split to rows in that case
    Urho3D::Vector<Urho3D::String> rows = eventData[P_MESSAGE].GetString().Split('\n');

    for (unsigned i = 0; i < rows.Size(); ++i)
        pendingRows_.Push(Urho3D::MakePair(level, rows[i]));

    if (autoVisibleOnError_ && level == Urho3D::LOG_ERROR && !IsVisible())
        SetVisible(true);
}

void Console::HandlePostUpdate(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData)
{
    // Ensure UI-elements are not detached
    if (!background_->GetParent())
    {
        Urho3D::UI* ui = GetSubsystem<Urho3D::UI>();
        Urho3D::UIElement* uiRoot = ui->GetRoot();
        uiRoot->AddChild(background_);
        uiRoot->AddChild(closeButton_);
    }

    if (!rowContainer_->GetNumItems() || pendingRows_.Empty())
        return;

    printing_ = true;
    rowContainer_->DisableLayoutUpdate();

    Urho3D::Text* text = 0;
    for (unsigned i = 0; i < pendingRows_.Size(); ++i)
    {
        rowContainer_->RemoveItem((unsigned)0);
        text = new Urho3D::Text(context_);
        text->SetText(pendingRows_[i].second_);
        // Make error message highlight
        text->SetStyle(pendingRows_[i].first_ == Urho3D::LOG_ERROR ? "ConsoleHighlightedText" : "ConsoleText");
        rowContainer_->AddItem(text);
    }

    pendingRows_.Clear();

    rowContainer_->EnsureItemVisibility(text);
    rowContainer_->EnableLayoutUpdate();
    rowContainer_->UpdateLayout();
    UpdateElements();   // May need to readjust the height due to scrollbar visibility changes
    printing_ = false;
}

}//ZMGUI
