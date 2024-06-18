#include <WinUser.h>
#include <string>
#include <string.h>
#include <processthreadsapi.h>
#include "Imgui/imgui.h"

#define IM_ARRAYSIZE(_ARR)          ((int)(sizeof(_ARR)/sizeof(*_ARR)))

namespace hotkeys
{
	int aimkey;
}

static int keystatus = 0;

void ChangeKey(void* blank)
{
	keystatus = 1;
	while (true)
	{
		for (int i = 0; i < 0x87; i++)
		{
			if (GetKeyState(i) & 0x8000)
			{
				hotkeys::aimkey = i;
				keystatus = 0;
				return;
			}
		}
	}
}

static const char* keyNames[] =
{
	"",
	"left mouse",
	"right mouse",
	"cancel",
	"middle mouse",
	"mouse 5",
	"mouse 4",
	"",
	"backspace",
	"tab",
	"",
	"",
	"clear",
	"enter",
	"",
	"",
	"shift",
	"control",
	"alt",
	"pause",
	"caps",
	"",
	"",
	"",
	"",
	"",
	"",
	"escape",
	"",
	"",
	"",
	"",
	"space",
	"page Up",
	"page Down",
	"end",
	"home",
	"left",
	"up",
	"right",
	"down",
	"",
	"",
	"",
	"print",
	"insert",
	"delete",
	"",
	"0",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"a",
	"b",
	"c",
	"d",
	"e",
	"f",
	"g",
	"h",
	"i",
	"j",
	"k",
	"l",
	"m",
	"n",
	"o",
	"p",
	"q",
	"r",
	"s",
	"t",
	"u",
	"v",
	"w",
	"x",
	"y",
	"z",
	"",
	"",
	"",
	"",
	"",
	"numpad 0",
	"numpad 1",
	"numpad 2",
	"numpad 3",
	"numpad 4",
	"numpad 5",
	"numpad 6",
	"numpad 7",
	"numpad 8",
	"numpad 9",
	"multiply",
	"add",
	"",
	"subtract",
	"decimal",
	"divide",
	"f1",
	"f2",
	"f3",
	"f4",
	"f5",
	"f6",
	"f7",
	"f8",
	"f9",
	"f10",
	"f11",
	"f12",
};
static bool Items_ArrayGetter(void* data, int idx, const char** out_text)
{
	const char* const* items = (const char* const*)data;
	if (out_text)
		*out_text = items[idx];
	return true;
}
void HotkeyButton(int aimkey, void* changekey, int status)
{
	const char* preview_value = NULL;
	if (aimkey >= 0 && aimkey < IM_ARRAYSIZE(keyNames))
		Items_ArrayGetter(keyNames, aimkey, &preview_value);

	std::string aimkeys;
	if (preview_value == NULL)
		aimkeys = "select a key";
	else
		aimkeys = preview_value;

	if (status == 1)
	{
		aimkeys = "press any key";
	}
	if (ImGui::Button(aimkeys.c_str(), ImVec2(125, 20)))
	{
		if (status == 0)
		{
			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)changekey, nullptr, 0, nullptr);
		}
	}
}