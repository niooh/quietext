#pragma once

#include <windows.h>
#include <string>

namespace editor {

enum {
    ID_EDIT = 100,
    ID_FILE_NEW = 1000,
    ID_FILE_OPEN,
    ID_FILE_SAVE,
    ID_FILE_SAVEAS,
    ID_FILE_CLOSE,
    ID_FILE_EXIT,
    ID_EDIT_UNDO,
    ID_EDIT_CUT,
    ID_EDIT_COPY,
    ID_EDIT_PASTE,
    ID_EDIT_SELECTALL,
    ID_HELP_ABOUT,
};

HMENU BuildMenu();

void Attach(HWND mainWnd, HWND editCtrl);

bool OpenPath(const std::wstring& path);

bool Save();
bool SaveAs();

bool ConfirmSaveIfDirty();

void NewDocument();
void CloseDocument();

void OnCommand(UINT id);
void OnEditChange();

bool DoOpenDialog(std::wstring& out);

void ApplyTitle();

} // namespace editor
