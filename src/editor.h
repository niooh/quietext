#pragma once
#include <windows.h>
#include <string>

namespace editor {

// 控件 / 菜单命令 ID
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

// 在 Edit 控件创建之后调用，绑定主窗口和控件、设置字体、刷新标题
void Attach(HWND mainWnd, HWND editCtrl);

// 打开指定路径：存在则读取；不存在则进入"未命名新建"状态（首次保存才落盘）
bool OpenPath(const std::wstring& path);

// 保存到当前路径（若无路径则先弹另存为）
bool Save();
// 弹出另存为对话框并更新路径（不写入内容）
bool SaveAs();
// 当前有未保存修改时询问用户；返回 false 表示"取消当前操作"
bool ConfirmSaveIfDirty();

// 命令入口（Ctrl+N / Ctrl+Q 等都走这里）
void NewDocument();      // 清空为空白未命名文档
void CloseDocument();    // 关闭当前文档，留空白未命名状态（不退出程序）
void OnCommand(UINT id);

// EN_CHANGE 通知
void OnEditChange();

// 打开文件对话框（也供 main 无参启动时使用）
bool DoOpenDialog(std::wstring& out);

// 刷新标题栏
void ApplyTitle();

}
