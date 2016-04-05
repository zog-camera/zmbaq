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

#include "Urho3D/Core/Object.h"

namespace Urho3D
{
class Button;
class BorderImage;
class DropDownList;
class Engine;
class Font;
class LineEdit;
class ListView;
class Text;
class UIElement;
class XMLFile;
}

namespace ZMGUI
{

/// %Console window with log history and command line prompt.
class Console : public Urho3D::Object
{
    URHO3D_OBJECT(Console, Urho3D::Object);

public:
    /// Construct.
    Console(Urho3D::Context* context);
    /// Destruct.
    ~Console();

    /// Set UI elements' style from an XML file.
    void SetDefaultStyle(Urho3D::XMLFile* style);
    /// Show or hide.
    void SetVisible(bool enable);
    /// Toggle visibility.
    void Toggle();

    /// Automatically set console to visible when receiving an error log message.
    void SetAutoVisibleOnError(bool enable) { autoVisibleOnError_ = enable; }

    /// Set the command interpreter.
    void SetCommandInterpreter(const Urho3D::String& interpreter) { commandInterpreter_ = interpreter; }

    /// Set number of buffered rows.
    void SetNumBufferedRows(unsigned rows);
    /// Set number of displayed rows.
    void SetNumRows(unsigned rows);
    /// Set command history maximum size, 0 disables history.
    void SetNumHistoryRows(unsigned rows);
    /// Set whether to automatically focus the line edit when showing. Default true on desktops and false on mobile devices, as on mobiles it would pop up the screen keyboard.
    void SetFocusOnShow(bool enable);
    /// Update elements to layout properly. Call this after manually adjusting the sub-elements.
    void UpdateElements();

    /// Return the UI style file.
    Urho3D::XMLFile* GetDefaultStyle() const;

    /// Return the background element.
    Urho3D::BorderImage* GetBackground() const { return background_; }

    /// Return the line edit element.
    Urho3D::LineEdit* GetLineEdit() const { return lineEdit_; }

    /// Return the close butoon element.
    Urho3D::Button* GetCloseButton() const { return closeButton_; }

    /// Return whether is visible.
    bool IsVisible() const;

    /// Return true when console is set to automatically visible when receiving an error log message.
    bool IsAutoVisibleOnError() const { return autoVisibleOnError_; }

    /// Return the last used command interpreter.
    const Urho3D::String& GetCommandInterpreter() const { return commandInterpreter_; }

    /// Return number of buffered rows.
    unsigned GetNumBufferedRows() const;

    /// Return number of displayed rows.
    unsigned GetNumRows() const { return displayedRows_; }

    /// Copy selected rows to system clipboard.
    void CopySelectedRows() const;

    /// Return history maximum size.
    unsigned GetNumHistoryRows() const { return historyRows_; }

    /// Return current history position.
    unsigned GetHistoryPosition() const { return historyPosition_; }

    /// Return history row at index.
    const Urho3D::String& GetHistoryRow(unsigned index) const;

    /// Return whether automatically focuses the line edit when showing.
    bool GetFocusOnShow() const { return focusOnShow_; }

private:
    /// Populate the command line interpreters that could handle the console command.
    bool PopulateInterpreter();
    /// Handle interpreter being selected on the drop down list.
    void HandleInterpreterSelected(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData);
    /// Handle enter pressed on the line edit.
    void HandleTextFinished(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData);
    /// Handle unhandled key on the line edit for scrolling the history.
    void HandleLineEditKey(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData);
    /// Handle close button being pressed.
    void HandleCloseButtonPressed(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData);
    /// Handle rendering window resize.
    void HandleScreenMode(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData);
    /// Handle a log message.
    void HandleLogMessage(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData);
    /// Handle the application post-update.
    void HandlePostUpdate(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData);

    /// Auto visible on error flag.
    bool autoVisibleOnError_;
    /// Background.
    Urho3D::SharedPtr<Urho3D::BorderImage> background_;
    /// Container for text rows.
    Urho3D::ListView* rowContainer_;
    /// Container for the command line.
    Urho3D::UIElement* commandLine_;
    /// Interpreter drop down list.
    Urho3D::DropDownList* interpreters_;
    /// Line edit.
    Urho3D::LineEdit* lineEdit_;
    /// Close button.
    Urho3D::SharedPtr<Urho3D::Button> closeButton_;
    /// Last used command interpreter.
    Urho3D::String commandInterpreter_;
    /// Command history.
    Urho3D::Vector<Urho3D::String> history_;
    /// Pending log message rows.
    Urho3D::Vector<Urho3D::Pair<int, Urho3D::String> > pendingRows_;
    /// Current row being edited.
    Urho3D::String currentRow_;
    /// Maximum displayed rows.
    unsigned displayedRows_;
    /// Command history maximum rows.
    unsigned historyRows_;
    /// Command history current position.
    unsigned historyPosition_;
    /// Flag when printing messages to prevent endless loop.
    bool printing_;
    /// Flag for automatically focusing the line edit on showing the console.
    bool focusOnShow_;
};

}//ZMGUI
