#pragma once
#include "zcheats.h"
#include "driver.h"
#include "d3d9_x.h"
#include "xor.hpp"
#include <Windows.h>
#include <vector>
#include <dwmapi.h>
#include <string>
#include "Keybind.h"
#include "skStr.h"
#include "offsets.h"
#include "lazy.h"
#include "xstring"
#include "skStr.h"
#include "settings.cpp"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "Imgui/glfw3.h"
#include <urlmon.h>	
#pragma comment(lib, "urlmon.lib")

ImFont* m_pFont;
DWORD_PTR Uworld;
DWORD_PTR 余ChinaWorld;
DWORD_PTR LocalPawn;
DWORD_PTR PlayerState;
DWORD_PTR Localplayer;
DWORD_PTR Rootcomp;
DWORD_PTR PlayerController;
DWORD_PTR Persistentlevel;
uintptr_t PlayerCameraManager;
Vector3 localactorpos;
uint64_t TargetPawn;
int localplayerID;
RECT GameRect = { NULL };
D3DPRESENT_PARAMETERS d3dpp;
DWORD ScreenCenterX;
DWORD ScreenCenterY;
static void setup_window();
static void xInitD3d();
static void xMainLoop();
static void xShutdown();
static LRESULT CALLBACK WinProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static HWND Window = NULL;
IDirect3D9Ex* p_Object = NULL;
static LPDIRECT3DDEVICE9 D3dDevice = NULL;
static LPDIRECT3DVERTEXBUFFER9 TriBuf = NULL;
int center_x = GetSystemMetrics(0) / 2 - 3;
int center_y = GetSystemMetrics(1) / 2 - 3;

#define BONE_HEAD_ID (109)
#define BONE_NECK_ID (66)
#define BONE_CHEST_ID (37)
#define BONE_PELVIS_ID (2)

Vector3 GetBoneWithRotation(uintptr_t mesh, int bone_id)
{
	uintptr_t bone_array =ReadBizzy<uintptr_t>(mesh + OFFSETS::BoneArray);
	if (bone_array == NULL) bone_array =ReadBizzy<uintptr_t>(mesh + OFFSETS::BoneArray + 0x10);
	FTransform bone =ReadBizzy<FTransform>(bone_array + (bone_id * 0x60));
	FTransform component_to_world =ReadBizzy<FTransform>(mesh + OFFSETS::ComponetToWorld);
	D3DMATRIX matrix = MatrixMultiplication(bone.ToMatrixWithScale(), component_to_world.ToMatrixWithScale());
	return Vector3(matrix._41, matrix._42, matrix._43);
}

D3DXMATRIX Matrix(Vector3 rot, Vector3 origin = Vector3(0, 0, 0))
{
	float radPitch = (rot.x * float(M_PI) / 180.f);
	float radYaw = (rot.y * float(M_PI) / 180.f);
	float radRoll = (rot.z * float(M_PI) / 180.f);

	float SP = sinf(radPitch);
	float CP = cosf(radPitch);
	float SY = sinf(radYaw);
	float CY = cosf(radYaw);
	float SR = sinf(radRoll);
	float CR = cosf(radRoll);

	D3DMATRIX matrix;
	matrix.m[0][0] = CP * CY;
	matrix.m[0][1] = CP * SY;
	matrix.m[0][2] = SP;
	matrix.m[0][3] = 0.f;

	matrix.m[1][0] = SR * SP * CY - CR * SY;
	matrix.m[1][1] = SR * SP * SY + CR * CY;
	matrix.m[1][2] = -SR * CP;
	matrix.m[1][3] = 0.f;

	matrix.m[2][0] = -(CR * SP * CY + SR * SY);
	matrix.m[2][1] = CY * SR - CR * SP * SY;
	matrix.m[2][2] = CR * CP;
	matrix.m[2][3] = 0.f;

	matrix.m[3][0] = origin.x;
	matrix.m[3][1] = origin.y;
	matrix.m[3][2] = origin.z;
	matrix.m[3][3] = 1.f;

	return matrix;
}


struct Camera
{
	Vector3 Location;
	Vector3 Rotation;
	float FieldOfView;
};

Camera get_view_point()
{
	Camera camera;

	auto location_pointer =ReadBizzy<uintptr_t>(Uworld + 0x110);
	auto rotation_pointer =ReadBizzy<uintptr_t>(Uworld + 0x120);

	struct FNRot
	{
		double a; //0x0000
		char pad_0008[24]; //0x0008
		double b; //0x0020
		char pad_0028[424]; //0x0028
		double c; //0x01D0
	}fnRot;

	fnRot.a =ReadBizzy<double>(rotation_pointer);
	fnRot.b =ReadBizzy<double>(rotation_pointer + 0x20);
	fnRot.c =ReadBizzy<double>(rotation_pointer + 0x1d0);

	camera.Location =ReadBizzy<Vector3>(location_pointer);
	camera.Rotation.x = asin(fnRot.c) * (180.0 / M_PI);
	camera.Rotation.y = ((atan2(fnRot.a * -1, fnRot.b) * (180.0 / M_PI)) * -1) * -1;
	camera.FieldOfView =ReadBizzy<float>((uintptr_t)PlayerController + 0x394) * 90.f;

	return camera;
}

Vector3 ProjectWorldToScreen(Vector3 WorldLocation)
{
	Camera ViewPoint = get_view_point();
	D3DMATRIX tempMatrix = Matrix(ViewPoint.Rotation);
	Vector3 vAxisX = Vector3(tempMatrix.m[0][0], tempMatrix.m[0][1], tempMatrix.m[0][2]);
	Vector3 vAxisY = Vector3(tempMatrix.m[1][0], tempMatrix.m[1][1], tempMatrix.m[1][2]);
	Vector3 vAxisZ = Vector3(tempMatrix.m[2][0], tempMatrix.m[2][1], tempMatrix.m[2][2]);
	Vector3 vDelta = WorldLocation - ViewPoint.Location;
	Vector3 vTransformed = Vector3(vDelta.Dot(vAxisY), vDelta.Dot(vAxisZ), vDelta.Dot(vAxisX));
	if (vTransformed.z < 1.f)
		vTransformed.z = 1.f;
	return Vector3((Width / 2.0f) + vTransformed.x * (((Width / 2.0f) / tanf(ViewPoint.FieldOfView * (float)M_PI / 360.f))) / vTransformed.z, (Height / 2.0f) - vTransformed.y * (((Width / 2.0f) / tanf(ViewPoint.FieldOfView * (float)M_PI / 360.f))) / vTransformed.z, 0);
}




float DrawOutlinedText(ImFont* pFont, const std::string& text, const ImVec2& pos, float size, ImU32 color, bool center)
{
	std::stringstream stream(text);
	std::string line;

	float y = 0.0f;
	int i = 0;

	while (std::getline(stream, line))
	{
		ImVec2 textSize = pFont->CalcTextSizeA(size, FLT_MAX, 0.0f, line.c_str());

		if (center)
		{
			ImGui::GetOverlayDrawList()->AddText(pFont, size, ImVec2((pos.x - textSize.x / 2.0f) + 1, (pos.y + textSize.y * i) + 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
			ImGui::GetOverlayDrawList()->AddText(pFont, size, ImVec2((pos.x - textSize.x / 2.0f) - 1, (pos.y + textSize.y * i) - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
			ImGui::GetOverlayDrawList()->AddText(pFont, size, ImVec2((pos.x - textSize.x / 2.0f) + 1, (pos.y + textSize.y * i) - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
			ImGui::GetOverlayDrawList()->AddText(pFont, size, ImVec2((pos.x - textSize.x / 2.0f) - 1, (pos.y + textSize.y * i) + 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());

			ImGui::GetOverlayDrawList()->AddText(pFont, size, ImVec2(pos.x - textSize.x / 2.0f, pos.y + textSize.y * i), ImGui::GetColorU32(color), line.c_str());
		}
		else
		{
			ImGui::GetOverlayDrawList()->AddText(pFont, size, ImVec2((pos.x) + 1, (pos.y + textSize.y * i) + 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
			ImGui::GetOverlayDrawList()->AddText(pFont, size, ImVec2((pos.x) - 1, (pos.y + textSize.y * i) - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
			ImGui::GetOverlayDrawList()->AddText(pFont, size, ImVec2((pos.x) + 1, (pos.y + textSize.y * i) - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
			ImGui::GetOverlayDrawList()->AddText(pFont, size, ImVec2((pos.x) - 1, (pos.y + textSize.y * i) + 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());

			ImGui::GetOverlayDrawList()->AddText(pFont, size, ImVec2(pos.x, pos.y + textSize.y * i), ImGui::GetColorU32(color), line.c_str());
		}

		y = pos.y + textSize.y * (i + 1);
		i++;
	}
	return y;
}

void DrawText1(int x, int y, const char* str, RGBA* color)
{
	ImFont a;
	std::string utf_8_1 = std::string(str);
	std::string utf_8_2 = string_To_UTF8(utf_8_1);
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x, y), ImGui::ColorConvertFloat4ToU32(ImVec4(color->G / 255.0, color->R / 255.0, color->B / 255.0, color->A / 255.0)), utf_8_2.c_str());
}
void DrawLine(int x1, int y1, int x2, int y2, RGBA* color, int thickness)
{
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), thickness);
}
void DrawCircle(int x, int y, int radius, RGBA* color, int segments)
{
	ImGui::GetOverlayDrawList()->AddCircle(ImVec2(x, y), radius, ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), segments);
}
void DrawBox(float X, float Y, float W, float H, ImU32 Col)
{
	ImGui::GetOverlayDrawList()->AddRect(ImVec2(X + 1, Y + 1), ImVec2(((X + W) - 1), ((Y + H) - 1)), Col);
	ImGui::GetOverlayDrawList()->AddRect(ImVec2(X, Y), ImVec2(X + W, Y + H), Col);
}
void DrawFilledRect(int x, int y, int w, int h, ImU32 color)
{
	ImGui::GetOverlayDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), color, 0, 0);
}
void DrawCorneredBox(int X, int Y, int W, int H, const ImU32& color, int thickness) {
	float lineW = (W / 3);
	float lineH = (H / 3);

	if (outlined) {
		ImGui::GetOverlayDrawList()->AddLine(ImVec2(X, Y), ImVec2(X, Y + lineH), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), 3);
		ImGui::GetOverlayDrawList()->AddLine(ImVec2(X, Y), ImVec2(X + lineW, Y), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), 3);
		ImGui::GetOverlayDrawList()->AddLine(ImVec2(X + W - lineW, Y), ImVec2(X + W, Y), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), 3);
		ImGui::GetOverlayDrawList()->AddLine(ImVec2(X + W, Y), ImVec2(X + W, Y + lineH), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), 3);
		ImGui::GetOverlayDrawList()->AddLine(ImVec2(X, Y + H - lineH), ImVec2(X, Y + H), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), 3);
		ImGui::GetOverlayDrawList()->AddLine(ImVec2(X, Y + H), ImVec2(X + lineW, Y + H), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), 3);
		ImGui::GetOverlayDrawList()->AddLine(ImVec2(X + W - lineW, Y + H), ImVec2(X + W, Y + H), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), 3);
		ImGui::GetOverlayDrawList()->AddLine(ImVec2(X + W, Y + H - lineH), ImVec2(X + W, Y + H), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), 3);
	}
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X, Y), ImVec2(X, Y + lineH), ImGui::GetColorU32(color), thickness);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X, Y), ImVec2(X + lineW, Y), ImGui::GetColorU32(color), thickness);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X + W - lineW, Y), ImVec2(X + W, Y), ImGui::GetColorU32(color), thickness);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X + W, Y), ImVec2(X + W, Y + lineH), ImGui::GetColorU32(color), thickness);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X, Y + H - lineH), ImVec2(X, Y + H), ImGui::GetColorU32(color), thickness);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X, Y + H), ImVec2(X + lineW, Y + H), ImGui::GetColorU32(color), thickness);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X + W - lineW, Y + H), ImVec2(X + W, Y + H), ImGui::GetColorU32(color), thickness);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X + W, Y + H - lineH), ImVec2(X + W, Y + H), ImGui::GetColorU32(color), thickness);
}
void DrawBox(int X, int Y, int W, int H, const ImU32& color, int thickness) {
	float lineW = (W / 1);
	float lineH = (H / 1);
	ImDrawList* Drawlist = ImGui::GetOverlayDrawList();
	if (outlined)
	{
		Drawlist->AddLine(ImVec2(X, Y), ImVec2(X, Y + lineH), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), 3);
		Drawlist->AddLine(ImVec2(X, Y), ImVec2(X + lineW, Y), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), 3);
		Drawlist->AddLine(ImVec2(X + W - lineW, Y), ImVec2(X + W, Y), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), 3);
		Drawlist->AddLine(ImVec2(X + W, Y), ImVec2(X + W, Y + lineH), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), 3);
		Drawlist->AddLine(ImVec2(X, Y + H - lineH), ImVec2(X, Y + H), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), 3);
		Drawlist->AddLine(ImVec2(X, Y + H), ImVec2(X + lineW, Y + H), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), 3);
		Drawlist->AddLine(ImVec2(X + W - lineW, Y + H), ImVec2(X + W, Y + H), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), 3);
		Drawlist->AddLine(ImVec2(X + W, Y + H - lineH), ImVec2(X + W, Y + H), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), 3);
	}
	Drawlist->AddLine(ImVec2(X, Y), ImVec2(X, Y + lineH), ImGui::GetColorU32(color), box_thickness);
	Drawlist->AddLine(ImVec2(X, Y), ImVec2(X + lineW, Y), ImGui::GetColorU32(color), box_thickness);
	Drawlist->AddLine(ImVec2(X + W - lineW, Y), ImVec2(X + W, Y), ImGui::GetColorU32(color), box_thickness);
	Drawlist->AddLine(ImVec2(X + W, Y), ImVec2(X + W, Y + lineH), ImGui::GetColorU32(color), box_thickness);
	Drawlist->AddLine(ImVec2(X, Y + H - lineH), ImVec2(X, Y + H), ImGui::GetColorU32(color), box_thickness);
	Drawlist->AddLine(ImVec2(X, Y + H), ImVec2(X + lineW, Y + H), ImGui::GetColorU32(color), box_thickness);
	Drawlist->AddLine(ImVec2(X + W - lineW, Y + H), ImVec2(X + W, Y + H), ImGui::GetColorU32(color), box_thickness);
	Drawlist->AddLine(ImVec2(X + W, Y + H - lineH), ImVec2(X + W, Y + H), ImGui::GetColorU32(color), box_thickness);
}



typedef struct _FNlEntity
{
	uint64_t Actor;
	int ID;
	uint64_t mesh;
}FNlEntity;
std::vector<FNlEntity> entityList;

bool IsVisible(uintptr_t mesh)
{
	float LastSumbitTime =ReadBizzy<float>(mesh + 0x358);
	float LastRenderTimeOnScreen =ReadBizzy<float>(mesh + 0x360);
	bool Visible = LastRenderTimeOnScreen + 0.06f >= LastSumbitTime;
	return Visible;
}
#include "Auth.hpp"
using namespace std;
using namespace KeyAuth;

std::string name = ("name");
std::string ownerid = ("ownerid");
std::string secret = ("secret");
std::string version = ("1.0");
std::string url = ("https://keyauth.win/api/1.2/"); // change if you're self-hosting
std::string path = (""); //optional, set a path if you're using the token validation setting

api KeyAuthApp(name, ownerid, secret, version, url, path);
int main(int argc, const char* argv[])
{ 

	KeyAuthApp.init();
	if (!KeyAuthApp.response.success)
	{
		std::cout << ("\n Status: ") << KeyAuthApp.response.message;
		Sleep(1500);
		exit(1);
	}
	std::string key;

	std::cout << ("\n Enter license: ");
	std::cin >> key;
	KeyAuthApp.license(key);
	if (!KeyAuthApp.response.success)
	{
		std::cout << ("\n Status: ") << KeyAuthApp.response.message;
		Sleep(1500);
		exit(1);
	}
	system("cls");
	Beep(500, 500);
	while (hwnd == NULL)
	{
		XorS(wind, "Fortnite  ");
		hwnd = FindWindowA(0, wind.decrypt());
		Sleep(100);
	}
	GetWindowThreadProcessId(hwnd, &processID);
	if (driver->Init(FALSE)) {
		driver->Attach(processID);
		base_address = driver->GetModuleBase(L"FortniteClient-Win64-Shipping.exe");
		printf("base addy %p", base_address);
	}
	setup_window();
	xInitD3d();
	xMainLoop();
	xShutdown();
	return 0;
}



void SetWindowToTarget()
{
	while (true)
	{
		if (hwnd)
		{
			ZeroMemory(&GameRect, sizeof(GameRect));
			GetWindowRect(hwnd, &GameRect);
			Width = GameRect.right - GameRect.left;
			Height = GameRect.bottom - GameRect.top;
			DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);

			if (dwStyle & WS_BORDER)
			{
				GameRect.top += 32;
				Height -= 39;
			}
			ScreenCenterX = Width / 2;
			ScreenCenterY = Height / 2;
			MoveWindow(Window, GameRect.left, GameRect.top, Width, Height, true);
		}
		else
		{
			exit(0);
		}
	}
}



const MARGINS Margin = { -1 };
HWND game_wnd;
int screen_width;
int screen_height;

void setup_window()
{
	WNDCLASSEX win_class = {
		sizeof(WNDCLASSEX),
		0,
		WinProc,
		0,
		0,
		nullptr,
		LoadIcon(nullptr, IDI_APPLICATION),
		LoadCursor(nullptr, IDC_ARROW),
		nullptr,
		nullptr,
		TEXT("lunarhook"),
		LoadIcon(nullptr, IDI_APPLICATION)
	};

	if (!RegisterClassEx(&win_class))
		exit(1);

	game_wnd = FindWindowW(NULL, NULL("Fortnite  "));

	if (game_wnd) {
		screen_width = 1920;
		screen_height = 100;
	}
	else
		exit(2);

	Window = CreateWindowExA(NULL, "lunarhook", "lunarhook", WS_POPUP | WS_VISIBLE, Width + 10, Height + 5, screen_width, screen_height, NULL, NULL, 0, NULL);
	DwmExtendFrameIntoClientArea(Window, &Margin);
	SetWindowLong(Window, GWL_EXSTYLE, (int)GetWindowLong(Window, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT);
	SetLayeredWindowAttributes(Window, RGB(0, 0, 0), 0, ULW_COLORKEY);
	SetLayeredWindowAttributes(Window, 0, 255, LWA_ALPHA);
	ShowWindow(Window, SW_SHOW);
	UpdateWindow(Window);
}

void xInitD3d()
{
	if (FAILED(Direct3DCreate9Ex(D3D_SDK_VERSION, &p_Object)))
		exit(3);

	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.BackBufferWidth = Width;
	d3dpp.BackBufferHeight = Height;
	d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
	d3dpp.MultiSampleQuality = D3DMULTISAMPLE_NONE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.hDeviceWindow = Window;
	d3dpp.Windowed = TRUE;

	p_Object->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, Window, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &D3dDevice);

	IMGUI_CHECKVERSION();

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	ImGui_ImplWin32_Init(Window);
	ImGui_ImplDX9_Init(D3dDevice);

	ImGui::StyleColorsDark();

	ImFontConfig font_config;
	font_config.PixelSnapH = false;
	font_config.OversampleH = 5;
	font_config.OversampleV = 5;
	font_config.RasterizerMultiply = 1.2f;

	static const ImWchar ranges[] =
	{
		0x0020, 0x00FF, // Basic Latin + Latin Supplement
		0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
		0x2DE0, 0x2DFF, // Cyrillic Extended-A
		0xA640, 0xA69F, // Cyrillic Extended-B
		0xE000, 0xE226, // icons
		0,
	};

	font_config.GlyphRanges = ranges;
	ImVec4 clear_color = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	// XorS(font, "C:\\Windows\\Fonts\\ariblk.ttf");
	XorS(font, "C:\\Windows\\Fonts\\arialbd.ttf");
	m_pFont = io.Fonts->AddFontFromFileTTF(font.decrypt(), 14.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
	p_Object->Release();
}

int SetMouseAbsPosition(DWORD x, DWORD y)
{
	typedef UINT(WINAPI* LPSENDINPUT)(UINT cInputs, LPINPUT pInputs, int cbSize);
	LPSENDINPUT pSendInput = (LPSENDINPUT)GetProcAddress(GetModuleHandleA("user32.dll"), "SendInput");
	if (pSendInput == NULL) {
		return 1;
	}

	// Move the mouse
	INPUT input = { 0 };
	input.type = INPUT_MOUSE;
	input.mi.dx = x;
	input.mi.dy = y;
	input.mi.dwFlags = MOUSEEVENTF_MOVE;
	// Set the mouse position

	return pSendInput(1, &input, sizeof(INPUT));
}

void aimbot(float x, float y)
{
	float ScreenCenterX = (Width / 2);
	float ScreenCenterY = (Height / 2);
	int AimSpeed = smooth;
	float TargetX = 0;
	float TargetY = 0;
	if (x != 0)
	{
		if (x > ScreenCenterX)
		{
			TargetX = -(ScreenCenterX - x);
			TargetX /= AimSpeed;
			if (TargetX + ScreenCenterX > ScreenCenterX * 2) TargetX = 0;
		}

		if (x < ScreenCenterX)
		{
			TargetX = x - ScreenCenterX;
			TargetX /= AimSpeed;
			if (TargetX + ScreenCenterX < 0) TargetX = 0;
		}
	}
	if (y != 0)
	{
		if (y > ScreenCenterY)
		{
			TargetY = -(ScreenCenterY - y);
			TargetY /= AimSpeed;
			if (TargetY + ScreenCenterY > ScreenCenterY * 2) TargetY = 0;
		}

		if (y < ScreenCenterY)
		{
			TargetY = y - ScreenCenterY;
			TargetY = y - ScreenCenterY;
			TargetY /= AimSpeed;
			if (TargetY + ScreenCenterY < 0) TargetY = 0;
		}
	}
	SetMouseAbsPosition(TargetX, TargetY);
	return;
}


void AimAt(DWORD_PTR entity)
{
	uint64_t currentactormesh =ReadBizzy<uint64_t>(entity + OFFSETS::Mesh);
	auto rootHead = GetBoneWithRotation(currentactormesh, hitbox);
	Vector3 rootHeadOut = ProjectWorldToScreen(rootHead);
		if (rootHeadOut.y != 0 || rootHeadOut.y != 0)
		{
			aimbot(rootHeadOut.x, rootHeadOut.y);
		}
}

double GetCrossDistance(double x1, double y1, double x2, double y2) {
	return sqrt(pow((x2 - x1), 2) + pow((y2 - y1), 2));
}
void OutlinedText(int x, int y, ImColor Color, const char* text)
{
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x - 1, y), ImColor(0, 0, 0), text);
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x + 1, y), ImColor(0, 0, 0), text);
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x, y - 1), ImColor(0, 0, 0), text);
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x, y + 1), ImColor(0, 0, 0), text);
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x, y), Color, text);
}
void DrawESP() {

	if (fovcircle) {
		if (Aimbot) {
			ImGui::GetOverlayDrawList()->AddCircle(ImVec2(ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y / 2), float(AimFOV), ImColor(0, 0, 0, 255), 100.0f, 3);
			ImGui::GetOverlayDrawList()->AddCircle(ImVec2(ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y / 2), float(AimFOV), ImColor(255, 255, 255, 255), 100.0f, 0.5);
		}
	}
	static const auto size = ImGui::GetIO().DisplaySize;
	static const auto center = ImVec2(size.x / 2, size.y / 2);
	if (hitboxpos == 0)
	{
		hitbox = 109; //head
	}
	else if (hitboxpos == 1)
	{
		hitbox = 66; //neck
	}
	else if (hitboxpos == 2)
	{
		hitbox = 37; //chest
	}
	else if (hitboxpos == 3)
	{
		hitbox = 2; //pelvis
	}
	if (aimkeypos == 0)
	{
		aimkey = 0x01;//left mouse button
	}
	else if (aimkeypos == 1)
	{
		aimkey = 0x02;//right mouse button
	}
	else if (aimkeypos == 2)
	{
		aimkey = 0x04;//middle mouse button
	}
	else if (aimkeypos == 3)
	{
		aimkey = 0x05;//x1 mouse button
	}
	else if (aimkeypos == 4)
	{
		aimkey = 0x06;//x2 mouse button
	}

#define A 0x41
#define B 0x42
#define C 0x43
#define D 0x44
#define E 0x45
#define F 0x46
#define G 0x47
#define H 0x48
#define I 0x49
#define J 0x4A
#define K 0x4B
#define L 0x4C
#define M 0x4D
#define N 0x4E
#define O 0x4F
#define P 0x50
#define Q 0x51
#define R 0x52
#define S 0x53
#define T 0x54
#define U 0x55
#define V 0x56
#define W 0x57
#define X 0x58
#define Y 0x59
#define Z 0x5A

	if (crosshair) {
		ImGui::GetForegroundDrawList()->AddLine(ImVec2(Width / 2 - 11, Height / 2), ImVec2(Width / 2 + 1, Height / 2), ImGui::GetColorU32({ 255., 255., 255., 255.f }), 1.0f);
		ImGui::GetForegroundDrawList()->AddLine(ImVec2(Width / 2 + 12, Height / 2), ImVec2(Width / 2 + 1, Height / 2), ImGui::GetColorU32({ 255., 255., 255., 255.f }), 1.0f);
		ImGui::GetForegroundDrawList()->AddLine(ImVec2(Width / 2, Height / 2 - 11), ImVec2(Width / 2, Height / 2), ImGui::GetColorU32({ 255., 255., 255., 255.f }), 1.0f);
		ImGui::GetForegroundDrawList()->AddLine(ImVec2(Width / 2, Height / 2 + 12), ImVec2(Width / 2, Height / 2), ImGui::GetColorU32({ 255., 255., 255., 255.f }), 1.0f);
	}

	if (fpsCounter)
	{
		auto draw_list = ImGui::GetBackgroundDrawList();
		auto watermark = ("p2c | pasted since 2019");
		auto watermark_size = ImGui::CalcTextSize(watermark);
		auto yOffset = 0;
		auto offset = 4;
		draw_list->AddRectFilled(ImVec2(6, 4 + yOffset), ImVec2((4 * 2) + watermark_size.x + 6, 6 + yOffset), ImColor(0.345f, 0.396f, 0.949f, 1.000f));
		draw_list->AddRectFilled(ImVec2(6, 6 + yOffset), ImVec2((4 * 2) + watermark_size.x + 6, 25 + yOffset), ImColor(0.2117647081613541f, 0.2235294133424759f, 0.2470588237047195f, 1.0f));
		draw_list->AddText(ImVec2(10, 6 + yOffset), ImColor(255, 255, 255, 255), watermark);
	}

	auto entityListCopy = entityList;
	float closestDistance = FLT_MAX;
	DWORD_PTR closestPawn = NULL;
	// new gworld fix
	余ChinaWorld = ReadBizzy<DWORD_PTR>(base_address + GWorld);
	    if (余ChinaWorld && GWorld) {
			std::srand(static_cast<unsigned int>(std::time(nullptr)));
			int chinaworld = 0xa1c3ef00;
			DWORD_PTR decrypt = GWorld * chinaworld;
			printf("decrypted address : ", &decrypt);
			std::cout << "[~] Successfully decrypted GWorld" << std::endl;
        }
		if (!余ChinaWorld) {
			std::cout << "[~] Failed [GWORLD/UWORLD]" << std::endl;
		}
	DWORD_PTR Gameinstance =ReadBizzy<DWORD_PTR>(Uworld + OFFSETS::Gameinstance);
	DWORD_PTR LocalPlayers =ReadBizzy<DWORD_PTR>(Gameinstance + OFFSETS::LocalPlayers);
	Localplayer =ReadBizzy<DWORD_PTR>(LocalPlayers);
	PlayerController =ReadBizzy<DWORD_PTR>(Localplayer + OFFSETS::PlayerController);
	LocalPawn =ReadBizzy<DWORD_PTR>(PlayerController + OFFSETS::LocalPawn);
	PlayerState =ReadBizzy<DWORD_PTR>(LocalPawn + OFFSETS::PlayerState);
	DWORD_PTR PlayerCameraManager =ReadBizzy<DWORD_PTR>(PlayerController + OFFSETS::Cameramanager);
	PlayerCameraManager =ReadBizzy<DWORD_PTR>(LocalPawn + PlayerCameraManager);
	Rootcomp =ReadBizzy<DWORD_PTR>(LocalPawn + OFFSETS::RootComponet);
	Persistentlevel =ReadBizzy<DWORD_PTR>(Uworld + OFFSETS::PersistentLevel);
	DWORD ActorCount =ReadBizzy<DWORD>(Persistentlevel + OFFSETS::ActorCount);
	DWORD_PTR AOFFSETS =ReadBizzy<DWORD_PTR>(Persistentlevel + OFFSETS::AActor);
	DWORD_PTR GameState =ReadBizzy<DWORD_PTR>(Uworld + OFFSETS::GameState);//gamestate
	DWORD_PTR PlayerArray =ReadBizzy<DWORD_PTR>(GameState + OFFSETS::PlayerArray);//playerarray

	int Num =ReadBizzy<int>(GameState + (OFFSETS::PlayerArray + sizeof(uintptr_t))); //reads the total number of player states in this array

	for (uint32_t i = 0; i < Num; i++)
	{
		auto player =ReadBizzy<uintptr_t>(PlayerArray + i * 0x8);
		auto CurrentActor =ReadBizzy<uintptr_t>(player + OFFSETS::Private);//PawnPrivate
		if (!CurrentActor) { continue; }
		uint64_t CurrentActorMesh =ReadBizzy<uint64_t>(CurrentActor + OFFSETS::Mesh);
		DWORD64 otherPlayerState =ReadBizzy<uint64_t>(CurrentActor + 0x290);
		auto entityListCopy = entityList;
		if (CurrentActor == LocalPawn) continue;
		Vector3 Headpos = GetBoneWithRotation(CurrentActorMesh, 67);
		localactorpos =ReadBizzy<Vector3>(Rootcomp + 0x120);
		float distance = localactorpos.Distance(Headpos) / ChangerFOV;
		Vector3 bone0 = GetBoneWithRotation(CurrentActorMesh, 0);
		Vector3 bottom = ProjectWorldToScreen(bone0);
		Vector3 Headbox = ProjectWorldToScreen(Vector3(Headpos.x, Headpos.y, Headpos.z + 15));
		Vector3 w2shead = ProjectWorldToScreen(Headpos);
		float CornerHeight = abs(Headbox.y - bottom.y);
		float CornerWidth = CornerHeight * 0.5;
		if (distance < VisDist)
		{
			if (fillbox)
			{
				DrawFilledRect(Headbox.x - (CornerWidth / 2), Headbox.y, CornerWidth, CornerHeight, IM_COL32(0, 0, 0, 125));
			}
			if (box)
			{
				DrawBox(Headbox.x - (CornerWidth / 2), Headbox.y, CornerWidth, CornerHeight, IM_COL32(255, 255, 255, 255), 1.5);
			}
			if (player_distance)
			{
				XorS(dst, "%.fm\n");
				char dist[32];
				sprintf_s(dist, dst.decrypt(), distance);
				DrawOutlinedText(m_pFont, dist, ImVec2(bottom.x, bottom.y - 20), 15, IM_COL32(255, 255, 255, 255), true);
			}
			if (visiblecheck)
			{
				if (IsVisible(CurrentActorMesh)) {
					XorS(dst, "targetable\n");
					char dist[32];
					sprintf_s(dist, dst.decrypt());
					DrawOutlinedText(m_pFont, dist, ImVec2(Headbox.x, Headbox.y - 0), 15, IM_COL32(255, 0, 0, 255), true);
				}
				if (IsVisible(CurrentActorMesh)) {
					XorS(dst, " ");
					char dist[32];
					sprintf_s(dist, dst.decrypt());
					DrawOutlinedText(m_pFont, dist, ImVec2(Headbox.x, Headbox.y - 0), 15, IM_COL32(255, 0, 0, 255), true);
				}
			}
			if (tracerpos == 0)
			{
				if (bottom_tracers) {
					DrawLine(Width / 2, Height, bottom.x, bottom.y, &Col.white, 1.5);
				}
			}
			else if (tracerpos == 1)
			{
				if (bottom_tracers) {
					ImGui::GetOverlayDrawList()->AddLine(ImVec2(center_x, center_y), ImVec2(Headbox.x, Headbox.y), ImGui::GetColorU32({ 1, 1, 1, 1 }), 1.5);
				}
			}
			else if (tracerpos == 2)
			{
				if (bottom_tracers) {
					ImGui::GetOverlayDrawList()->AddLine(ImVec2(Width / 2, Height - Height), ImVec2(Headbox.x, Headbox.y), ImColor(255, 255, 255), 1.5);
				}
			}
			if (skeletonesp)
			{
				Vector3 vHeadBone = GetBoneWithRotation(CurrentActorMesh, 68);
				Vector3 vHip = GetBoneWithRotation(CurrentActorMesh, 3);
				Vector3 vNeck = GetBoneWithRotation(CurrentActorMesh, 67);
				Vector3 vUpperArmLeft = GetBoneWithRotation(CurrentActorMesh, 9);
				Vector3 vUpperArmRight = GetBoneWithRotation(CurrentActorMesh, 38);
				Vector3 vLeftHand = GetBoneWithRotation(CurrentActorMesh, 33);
				Vector3 vRightHand = GetBoneWithRotation(CurrentActorMesh, 62);
				Vector3 vHandLeftDown = GetBoneWithRotation(CurrentActorMesh, 31);
				Vector3 vandRightDown = GetBoneWithRotation(CurrentActorMesh, 58);
				Vector3 vIndexLeft = GetBoneWithRotation(CurrentActorMesh, 32);
				Vector3 vIndexRight = GetBoneWithRotation(CurrentActorMesh, 42);
				Vector3 vMiddleLeft = GetBoneWithRotation(CurrentActorMesh, 14);
				Vector3 vMiddleRight = GetBoneWithRotation(CurrentActorMesh, 48);
				Vector3 vLeftHand1 = GetBoneWithRotation(CurrentActorMesh, 33);
				Vector3 vRightHand1 = GetBoneWithRotation(CurrentActorMesh, 62);
				Vector3 vRightThigh = GetBoneWithRotation(CurrentActorMesh, 78);
				Vector3 vLeftThigh = GetBoneWithRotation(CurrentActorMesh, 71);
				Vector3 vRightCalf = GetBoneWithRotation(CurrentActorMesh, 79);
				Vector3 vLeftCalf = GetBoneWithRotation(CurrentActorMesh, 72);
				Vector3 vLeftFoot = GetBoneWithRotation(CurrentActorMesh, 73);
				Vector3 vRightFoot = GetBoneWithRotation(CurrentActorMesh, 80);
				Vector3 vLeftHeel = GetBoneWithRotation(CurrentActorMesh, 75);
				Vector3 vRightHeel = GetBoneWithRotation(CurrentActorMesh, 82);
				Vector3 vHeadBoneOut = ProjectWorldToScreen(vHeadBone);
				Vector3 vHipOut = ProjectWorldToScreen(vHip);
				Vector3 vNeckOut = ProjectWorldToScreen(vNeck);
				Vector3 vUpperArmLeftOut = ProjectWorldToScreen(vUpperArmLeft);
				Vector3 vUpperArmRightOut = ProjectWorldToScreen(vUpperArmRight);
				Vector3 vLeftHandOut = ProjectWorldToScreen(vLeftHand);
				Vector3 vRightHandOut = ProjectWorldToScreen(vRightHand);
				Vector3 vLeftHandOut1 = ProjectWorldToScreen(vLeftHand1);
				Vector3 vRightHandOut1 = ProjectWorldToScreen(vRightHand1);
				Vector3 vHandLeftDownOut = ProjectWorldToScreen(vHandLeftDown);
				Vector3 vandRightDownOut = ProjectWorldToScreen(vandRightDown);
				Vector3 vLeftHeelOut = ProjectWorldToScreen(vLeftHeel);
				Vector3 vRightHeelOut = ProjectWorldToScreen(vRightHeel);
				Vector3 vRightThighOut = ProjectWorldToScreen(vRightThigh);
				Vector3 vLeftThighOut = ProjectWorldToScreen(vLeftThigh);
				Vector3 vRightCalfOut = ProjectWorldToScreen(vRightCalf);
				Vector3 vLeftCalfOut = ProjectWorldToScreen(vLeftCalf);
				Vector3 vLeftFootOut = ProjectWorldToScreen(vLeftFoot);
				Vector3 vRightFootOut = ProjectWorldToScreen(vRightFoot);
				Vector3 vElbowLeft = GetBoneWithRotation(CurrentActorMesh, 10);
				Vector3 vElbowRight = GetBoneWithRotation(CurrentActorMesh, 39);
				Vector3 vElbowLeftOut = ProjectWorldToScreen(vElbowLeft);
				Vector3 vElbowRightOut = ProjectWorldToScreen(vElbowRight);
				DrawLine(vHeadBoneOut.x, vHeadBoneOut.y, vNeckOut.x, vNeckOut.y, &Col.red, skeleton_thickness);
				DrawLine(vHipOut.x, vHipOut.y, vNeckOut.x, vNeckOut.y, &Col.red, skeleton_thickness);
				DrawLine(vUpperArmLeftOut.x, vUpperArmLeftOut.y, vNeckOut.x, vNeckOut.y, &Col.red, skeleton_thickness);
				DrawLine(vUpperArmRightOut.x, vUpperArmRightOut.y, vNeckOut.x, vNeckOut.y, &Col.red, skeleton_thickness);
				DrawLine(vLeftThighOut.x, vLeftThighOut.y, vHipOut.x, vHipOut.y, &Col.red, skeleton_thickness);
				DrawLine(vRightThighOut.x, vRightThighOut.y, vHipOut.x, vHipOut.y, &Col.red, skeleton_thickness);
				DrawLine(vLeftCalfOut.x, vLeftCalfOut.y, vLeftThighOut.x, vLeftThighOut.y, &Col.red, skeleton_thickness);
				DrawLine(vRightCalfOut.x, vRightCalfOut.y, vRightThighOut.x, vRightThighOut.y, &Col.red, skeleton_thickness);
				DrawLine(vLeftFootOut.x, vLeftFootOut.y, vLeftCalfOut.x, vLeftCalfOut.y, &Col.red, skeleton_thickness);
				DrawLine(vRightFootOut.x, vRightFootOut.y, vRightCalfOut.x, vRightCalfOut.y, &Col.red, skeleton_thickness);
				DrawLine(vElbowLeftOut.x, vElbowLeftOut.y, vUpperArmLeftOut.x, vUpperArmLeftOut.y, &Col.red, skeleton_thickness);
				DrawLine(vElbowRightOut.x, vElbowRightOut.y, vUpperArmRightOut.x, vUpperArmRightOut.y, &Col.red, skeleton_thickness);
				DrawLine(vLeftHandOut.x, vLeftHandOut.y, vElbowLeftOut.x, vElbowLeftOut.y, &Col.red, skeleton_thickness);
				DrawLine(vRightHandOut.x, vRightHandOut.y, vElbowRightOut.x, vElbowRightOut.y, &Col.red, skeleton_thickness);
			}
		}
		auto dx = w2shead.x - (Width / 2);
		auto dy = w2shead.y - (Height / 2);
		auto dist = sqrtf(dx * dx + dy * dy);
		if (dist < AimFOV && dist < closestDistance) {
			closestDistance = dist;
			closestPawn = CurrentActor;
			if (Aimbot)
			{
				if (aimbotvisibleonly) {
					if (IsVisible(CurrentActorMesh)) {
						ImGui::GetForegroundDrawList()->AddLine(ImVec2(Width / 2 - 11, Height / 2), ImVec2(Width / 2 + 1, Height / 2), ImGui::GetColorU32({ 255, 0, 0, 255 }), 1.0f);
						ImGui::GetForegroundDrawList()->AddLine(ImVec2(Width / 2 + 12, Height / 2), ImVec2(Width / 2 + 1, Height / 2), ImGui::GetColorU32({ 255, 0, 0, 255 }), 1.0f);
						ImGui::GetForegroundDrawList()->AddLine(ImVec2(Width / 2, Height / 2 - 11), ImVec2(Width / 2, Height / 2), ImGui::GetColorU32({ 255, 0, 0, 255 }), 1.0f);
						ImGui::GetForegroundDrawList()->AddLine(ImVec2(Width / 2, Height / 2 + 12), ImVec2(Width / 2, Height / 2), ImGui::GetColorU32({ 255, 0, 0, 255 }), 1.0f);
						if (Aimbot && closestPawn && GetAsyncKeyState(hotkeys::aimkey) < 0) {
							AimAt(closestPawn);
						}
					}
				}
				else {
					if (Aimbot && closestPawn && GetAsyncKeyState(hotkeys::aimkey) < 0) {
						AimAt(closestPawn);
					}
				}
			}
		}
	}
}


static int tabb = 0;
static int tab = 0;

namespace ImGui {
	IMGUI_API bool Tab(unsigned int index, const char* label, int* selected, float width = 120, float height = 40)
	{
		ImGuiStyle& style = ImGui::GetStyle();
		ImColor color = ImColor(11, 11, 11, 255)/*style.Colors[ImGuiCol_Button]*/;
		ImColor colortext = ImColor(85, 85, 85)/*style.Colors[ImGuiCol_Text]*/;
		ImColor colortextActive = ImColor(255, 255, 255)/*style.Colors[ImGuiCol_Text]*/;
		ImColor colortextHover = ImColor(255, 255, 255)/*style.Colors[ImGuiCol_Text]*/;
		ImColor colorActive = ImColor(11, 11, 11, 255); /*style.Colors[ImGuiCol_ButtonActive]*/;
		ImColor colorHover = ImColor(11, 11, 11, 255)/*style.Colors[ImGuiCol_ButtonHovered]*/;
		if (index == *selected)
		{
			style.Colors[ImGuiCol_Text] = colortextActive;
			//style.Colors[ImGuiCol_Text] = colortextHover;

			style.Colors[ImGuiCol_Button] = colorActive;
			style.Colors[ImGuiCol_ButtonActive] = colorActive;
			style.Colors[ImGuiCol_ButtonHovered] = colorActive;
		}
		else
		{
			style.Colors[ImGuiCol_Text] = colortext;

			style.Colors[ImGuiCol_Button] = color;
			style.Colors[ImGuiCol_ButtonActive] = colorActive;
			style.Colors[ImGuiCol_ButtonHovered] = colorHover;
		}

		if (ImGui::Button(label, ImVec2(width, height)))
			*selected = index;

		style.Colors[ImGuiCol_Text] = colortext;

		style.Colors[ImGuiCol_Button] = color;
		style.Colors[ImGuiCol_ButtonActive] = colorActive;
		style.Colors[ImGuiCol_ButtonHovered] = colorHover;
		ImDrawList* pDrawList;
		const auto& CurrentWindowPos = ImGui::GetWindowPos();
		const auto& pWindowDrawList = ImGui::GetWindowDrawList();
		ImVec2 P1, P2;
		if (tabb == 0) {
			P1 = ImVec2(126, 29); //25
			P1.x += CurrentWindowPos.x; // + 9
			P1.y += CurrentWindowPos.y;
			P2 = ImVec2(126, 54);
			P2.x += CurrentWindowPos.x;
			P2.y += CurrentWindowPos.y;
			pDrawList = pWindowDrawList;
			pDrawList->AddLine(P1, P2, ImColor(22, 118, 243, 255), 3);
		}
		if (tabb == 1) {
			P1 = ImVec2(126, 72);
			P1.x += CurrentWindowPos.x;
			P1.y += CurrentWindowPos.y;
			P2 = ImVec2(126, 98);
			P2.x += CurrentWindowPos.x;
			P2.y += CurrentWindowPos.y;
			pDrawList = pWindowDrawList;
			pDrawList->AddLine(P1, P2, ImColor(22, 118, 243, 255), 3);
		}
		if (tabb == 2) {
			P1 = ImVec2(126, 117);
			P1.x += CurrentWindowPos.x;
			P1.y += CurrentWindowPos.y;
			P2 = ImVec2(126, 143);
			P2.x += CurrentWindowPos.x;
			P2.y += CurrentWindowPos.y;
			pDrawList = pWindowDrawList;
			pDrawList->AddLine(P1, P2, ImColor(22, 118, 243, 255), 3);
		}
		if (tabb == 3) {
			P1 = ImVec2(126, 161);
			P1.x += CurrentWindowPos.x;//26
			P1.y += CurrentWindowPos.y;
			P2 = ImVec2(126, 187);
			P2.x += CurrentWindowPos.x;
			P2.y += CurrentWindowPos.y;
			pDrawList = pWindowDrawList;
			pDrawList->AddLine(P1, P2, ImColor(22, 118, 243, 255), 3);
		}
		return *selected == index;
	}
}


void render() {
	
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	if (GetAsyncKeyState(VK_INSERT) & 1) {
		ShowMenu = !ShowMenu;
	}
	if (ShowMenu)
	{
		static float customSliderValue1 = 0.5f;
		static float customSliderValue2 = 0.7f;
		ImGui::SetNextWindowSize(ImVec2(510, 400));
		ImGuiStyle* style = &ImGui::GetStyle();
		style->Colors[ImGuiCol_Text] = ImColor(230, 230, 230);
		style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.0f, 0.0263f, 0.0357f, 1.00f);
		style->Colors[ImGuiCol_WindowBg] = ImColor(24, 24, 24);
		style->Colors[ImGuiCol_ChildBg] = ImColor(14, 14, 14);
		style->Colors[ImGuiCol_PopupBg] = ImVec4(0.080f, 0.080f, 0.080f, 0.940f);
		style->Colors[ImGuiCol_Border] = ImColor(0, 0, 0);
		style->Colors[ImGuiCol_BorderShadow] = ImColor(1, 1, 1);
		style->Colors[ImGuiCol_FrameBg] = ImColor(24, 24, 24);
		style->Colors[ImGuiCol_FrameBgHovered] = ImColor(24, 24, 24);
		style->Colors[ImGuiCol_FrameBgActive] = ImColor(24, 24, 24);
		style->Colors[ImGuiCol_TitleBg] = ImVec4(0.102f, 0.090f, 0.122f, 1.000f);
		style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.102f, 0.090f, 0.122f, 1.000f);
		style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.102f, 0.090f, 0.122f, 1.000f);
		style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.0f, 0.263f, 0.357f, 1.00f);
		style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		style->Colors[ImGuiCol_CheckMark] = ImColor(105, 0, 255);
		style->Colors[ImGuiCol_SliderGrab] = ImColor(0, 242, 255);
		style->Colors[ImGuiCol_SliderGrabActive] = ImColor(0, 242, 255);
		style->Colors[ImGuiCol_Button] = ImColor(24, 24, 24);
		style->Colors[ImGuiCol_ButtonHovered] = ImColor(24, 24, 24);
		style->Colors[ImGuiCol_ButtonActive] = ImColor(24, 24, 24);
		style->Colors[ImGuiCol_Header] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
		style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
		style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
		style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		style->Colors[ImGuiCol_PlotLines] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
		style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
		style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
		style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
		style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
		style->Colors[ImGuiCol_Separator] = ImColor(105, 0, 255);

		static POINT Mouse;
		GetCursorPos(&Mouse);
		ImGui::GetForegroundDrawList()->AddCircleFilled(ImVec2(Mouse.x, Mouse.y), float(4), ImColor(255, 0, 0), 10);

		style->WindowPadding = ImVec2(7.000f, 7.000f);
		style->WindowRounding = 0.000f;
		style->FramePadding = ImVec2(4.000f, 0.000f);
		style->FrameRounding = 0.000f;
		style->WindowBorderSize = 1;
		style->FrameBorderSize = 0;
		style->ChildBorderSize = 1;

		ImGui::Begin(("Menu"), NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
		{
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(ImColor(47, 47, 47)));
			ImGui::Text("shitpaste_lunar | been ud since 1980");

			ImGui::BeginChild(("##aim1"), ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar);
			{
				ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(ImColor(25, 25, 25)));
				ImGui::BeginChild(("##aim1"), ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar);
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(ImColor(37, 37, 37)));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(ImColor(37, 37, 37)));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(ImColor(37, 37, 37)));
					ImGui::SetCursorPos(ImVec2(7, 20));
					ImGui::Tab(0, "aimbot", &tabb);

					ImGui::SetCursorPos(ImVec2(7, 65));
					ImGui::Tab(1, "visuals", &tabb);

					ImGui::SetCursorPos(ImVec2(7, 110));
					ImGui::Tab(2, "other", &tabb);

					if (tab == 0)
					ImGui::SetCursorPos(ImVec2(135, 5));
					ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(ImColor(37, 37, 37)));
					ImGui::BeginChild(("##631"), ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar);
					{
						ImGui::BeginChild(("##451"), ImVec2(318, 0), true, ImGuiWindowFlags_NoScrollbar);
						{
							ImGui::Text("aimbot");
							ImGui::Checkbox("enable", &Aimbot);
							if (Aimbot) {
								ImGui::Checkbox(skCrypt("fov circle"), &fovcircle);
								if (fovcircle) {
									ImGui::SliderFloat(skCrypt("fov size"), &AimFOV, 50, 800);
								}
								ImGui::SliderFloat(skCrypt("smoothness"), &smooth, 1, 10);
								ImGui::Checkbox("aimbot visible only", &aimbotvisibleonly);
								ImGui::Text("aimkey");
								ImGui::SameLine();
								HotkeyButton(hotkeys::aimkey, ChangeKey, keystatus);
								ImGui::Combo("aim bone", &hitboxpos, hitboxes, sizeof(hitboxes) / sizeof(*hitboxes));
							}
							ImGui::EndChild();
						}
						ImGui::EndChild();
					}

					if (tabb == 1)
					{
						ImGui::SetCursorPos(ImVec2(135, 5));
						ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(ImColor(37, 37, 37)));
						ImGui::BeginChild(("##aim1"), ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar);
						{
							ImGui::BeginChild(("##ai3m1"), ImVec2(159, 0), true, ImGuiWindowFlags_NoScrollbar);
							{
								    ImGui::Text("visuals");
									ImGui::Checkbox("2d box", &box);
									ImGui::Checkbox("filled box", &fillbox);
									ImGui::Checkbox("visible check", &visiblecheck);
									ImGui::Checkbox("snaplines", &bottom_tracers);
									ImGui::Checkbox("distance", &player_distance);
									ImGui::Checkbox("skeleton", &skeletonesp);
									ImGui::EndChild();
							}
								ImGui::SameLine();
								ImGui::BeginChild(("##ai3m31"), ImVec2(159, 0), true, ImGuiWindowFlags_NoScrollbar);
								{
									ImGui::Text("customization");
									if (skeletonesp) {
										ImGui::SliderFloat(skCrypt("skeleton thickness"), &skeleton_thickness, 1, 10);
									}
									if (box) {
										ImGui::Checkbox("outlined box", &outlined);
										ImGui::SliderFloat(skCrypt("box thickness"), &box_thickness, 1, 10);
									}
									if (bottom_tracers) {
										ImGui::Combo("tracer location", &tracerpos, tracerloc, sizeof(tracerloc) / sizeof(*tracerloc));
									}
									ImGui::EndChild();
							}
							ImGui::PopStyleColor();
						}
					}
					if (tabb == 2)
					{
						ImGui::SetCursorPos(ImVec2(135, 5));
						ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(ImColor(37, 37, 37)));
						ImGui::BeginChild(("##aim1"), ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar);
						{
							ImGui::BeginChild(("##ai3m1"), ImVec2(159, 0), true, ImGuiWindowFlags_NoScrollbar);
							{
								ImGui::Text("misc");
								ImGui::Checkbox(skCrypt("crosshair"), &crosshair);
								ImGui::EndChild();
							}
						}
						ImGui::PopStyleColor();
					}

					ImGui::PopStyleColor();
					ImGui::PopStyleColor();
					ImGui::PopStyleColor();
				}
				ImGui::PopStyleColor();
				ImGui::EndChild();
			}
		}
	}
	    ImGui::PopStyleColor();
		ImGui::End();
        DrawESP();
	ImGui::EndFrame();
	D3dDevice->SetRenderState(D3DRS_ZENABLE, false);
	D3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
	D3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, false);
	D3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);

	if (D3dDevice->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		D3dDevice->EndScene();
	}
	HRESULT result = D3dDevice->Present(NULL, NULL, NULL, NULL);

	if (result == D3DERR_DEVICELOST && D3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
	{
		ImGui_ImplDX9_InvalidateDeviceObjects();
		D3dDevice->Reset(&d3dpp);
		ImGui_ImplDX9_CreateDeviceObjects();
	}
}

MSG Message = { NULL };
int Loop = 0;
void xMainLoop()
{
	static RECT old_rc;
	ZeroMemory(&Message, sizeof(MSG));
	while (Message.message != WM_QUIT)
	{
		if (PeekMessage(&Message, Window, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}

		HWND hwnd_active = GetForegroundWindow();

		if (hwnd_active == hwnd) {
			HWND hwndtest = GetWindow(hwnd_active, GW_HWNDPREV);
			SetWindowPos(Window, hwndtest, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}

		RECT rc;
		POINT xy;

		ZeroMemory(&rc, sizeof(RECT));
		ZeroMemory(&xy, sizeof(POINT));
		GetClientRect(hwnd, &rc);
		ClientToScreen(hwnd, &xy);
		rc.left = xy.x;
		rc.top = xy.y;

		ImGuiIO& io = ImGui::GetIO();
		io.IniFilename = NULL;
		io.ImeWindowHandle = hwnd;
		io.DeltaTime = 1.0f / 60.0f;

		POINT p;
		GetCursorPos(&p);
		io.MousePos.x = p.x - xy.x;
		io.MousePos.y = p.y - xy.y;

		if (GetAsyncKeyState(VK_LBUTTON)) {
			io.MouseDown[0] = true;
			io.MouseClicked[0] = true;
			io.MouseClickedPos[0].x = io.MousePos.x;
			io.MouseClickedPos[0].x = io.MousePos.y;
		}
		else
			io.MouseDown[0] = false;

		if (rc.left != old_rc.left || rc.right != old_rc.right || rc.top != old_rc.top || rc.bottom != old_rc.bottom)
		{
			old_rc = rc;

			Width = rc.right;
			Height = rc.bottom;

			d3dpp.BackBufferWidth = Width;
			d3dpp.BackBufferHeight = Height;
			SetWindowPos(Window, (HWND)0, xy.x, xy.y, Width, Height, SWP_NOREDRAW);
			D3dDevice->Reset(&d3dpp);
		}
		render();
		if (Loop == 0) {
			XorS(base, "Process base address: %p.\n");
		}
		Loop = Loop + 1;
	}
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	DestroyWindow(Window);
}

LRESULT CALLBACK WinProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, Message, wParam, lParam))
		return true;

	switch (Message)
	{
	case WM_DESTROY:
		xShutdown();
		PostQuitMessage(0);
		exit(4);
		break;
	case WM_SIZE:
		if (D3dDevice != NULL && wParam != SIZE_MINIMIZED)
		{
			ImGui_ImplDX9_InvalidateDeviceObjects();
			d3dpp.BackBufferWidth = LOWORD(lParam);
			d3dpp.BackBufferHeight = HIWORD(lParam);
			HRESULT hr = D3dDevice->Reset(&d3dpp);
			if (hr == D3DERR_INVALIDCALL)
				IM_ASSERT(0);
			ImGui_ImplDX9_CreateDeviceObjects();
		}
		break;
	default:
		return DefWindowProc(hWnd, Message, wParam, lParam);
		break;
	}
	return 0;
}

void xShutdown()
{
	TriBuf->Release();
	D3dDevice->Release();
	p_Object->Release();

	DestroyWindow(Window);
	UnregisterClass("steam", NULL);
}

