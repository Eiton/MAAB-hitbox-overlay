#include "pch.h"
#include <d3d9.h>
#include <d3dx9.h>
#include <Windows.h>
#include <iostream>
#include <map>
#include "detours.h"
using namespace std;

#pragma comment(lib,"d3d9.lib")
#pragma comment(lib,"d3dx9.lib")

#pragma comment(lib, "detours.lib")


DWORD pid;
HANDLE phandle;
DWORD base_address = 0x00400000;
DWORD add_address = 0x049C4F08;
DWORD objStart;
bool inBattle = false;
std::map<DWORD, std::map<int, int>> objMap;
typedef HRESULT(_stdcall* f_EndScene)(IDirect3DDevice9* pDevice);
f_EndScene oEndScene;

typedef void(_stdcall* f_BattleCamera)();
f_BattleCamera oBattleCamera;

typedef int(__cdecl* f_GetHitboxType)(DWORD pt1, DWORD pt2);
f_GetHitboxType oGetHitboxType;

void* d3d9Device[119];
bool GetD3D9Device(void** pTable, size_t Size)
{
	std::cout << "[*] Trying to get DirectXDevice by new_creation method..\n";

	if (!pTable)
	{
		std::cout << "[-] pTable not set.\n";
		return false;
	}


	std::cout << "[*] Right before Direct3DCreate9.\n";
	IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (!pD3D)
	{
		std::cout << "[-] Direct3DCreate9 failed.\n";
		return false;
	}
	std::cout << "[+] Direct3DCreate9 successfull.\n";

	D3DPRESENT_PARAMETERS d3dpp = { 0 };
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = GetForegroundWindow();
	d3dpp.Windowed = ((GetWindowLong(d3dpp.hDeviceWindow, GWL_STYLE) & WS_POPUP) != 0) ? FALSE : TRUE;;

	std::cout << "[*] Got here..\n";

	IDirect3DDevice9* pDummyDevice = nullptr;
	HRESULT create_device_ret = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDummyDevice);
	std::cout << "[*] Return of CreateDevice:\n";
	std::cout << create_device_ret;
	std::cout << "\n";
	if (!pDummyDevice || FAILED(create_device_ret))
	{
		std::cout << "[-] CreateDevice failed\n";
		pD3D->Release();
		return false;
	}
	std::cout << "[+] CreateDevice successfull.\n";

	memcpy(pTable, *reinterpret_cast<void***>(pDummyDevice), Size);

	pDummyDevice->Release();
	pD3D->Release();

	std::cout << "[+] Success!\n";
	return true;
}

void DrawRectangle(IDirect3DDevice9* pDevice, const float x1, const float x2, const float y1, const float y2,const float z, D3DCOLOR innerColor, D3DCOLOR outerColor) {
	struct vertex
	{
		float x, y, z, rhw;
		DWORD color;
	};
	
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
	pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	pDevice->SetPixelShader(nullptr);
	pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	pDevice->SetTexture(0, nullptr);
	
	vertex vertices[] =
	{
		{ x1, y1, z, 1.F, innerColor },
		{ x1, y2, z, 1.F, innerColor },
		{ x2, y1, z, 1.F, innerColor },
		{ x2, y2, z, 1.F, innerColor },
	};
	pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(vertex)); 
	vertex outline[] =
	{
		{ x1, y1, z, 1.F, outerColor },
		{ x1, y2, z, 1.F, outerColor },
		{ x2, y2, z, 1.F, outerColor },
		{ x2, y1, z, 1.F, outerColor },
		{ x1, y1, z, 1.F, outerColor },
	};
	pDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, 4, outline, sizeof(vertex));
}
HRESULT _stdcall Hooked_EndScene(IDirect3DDevice9* pDevice)
{
	//Get Resolution
	DWORD resolutionX, resolutionY;
	ReadProcessMemory(phandle, (void*)(base_address + 0x4D9B484), &resolutionX, sizeof(resolutionX), 0);
	ReadProcessMemory(phandle, (void*)(base_address + 0x4D9B488), &resolutionY, sizeof(resolutionY), 0);

	// Get Camera pos, zoom
	float cameraPosX, cameraPosY, cameraZoom;

	ReadProcessMemory(phandle, (void*)(base_address + 0x4771ADC), &cameraPosX, sizeof(cameraPosX), 0);
	ReadProcessMemory(phandle, (void*)(base_address + 0x4771ADC + 0x4), &cameraPosY, sizeof(cameraPosY), 0);
	ReadProcessMemory(phandle, (void*)(base_address + 0x4771ADC + 0x58), &cameraZoom, sizeof(cameraZoom), 0);
	if (!objMap.empty() && inBattle == true) {
		
		std::map<DWORD, std::map<int, int>>::iterator it;
		for (it = objMap.begin(); it != objMap.end(); ++it) {
			DWORD objAddress = it->first;
			//Not used
			//float posX, posY;
			//ReadProcessMemory(phandle, (void*)(objAddress + 0xc4), &posX, sizeof(posX), 0);
			//ReadProcessMemory(phandle, (void*)(objAddress + 0xc8), &posY, sizeof(posY), 0);

			std::map<int, int> boxMap = it->second;
			std::map<int, int>::iterator it2 = boxMap.begin();
			for (it2 = ++it2; it2 != boxMap.end(); ++it2) {
				int index = it2->first;
				int type = it2->second;
				signed short x1, x2, y1, y2;
				ReadProcessMemory(phandle, (void*)(objAddress + 0x380 + 0x14 * index), &x1, sizeof(x1), 0);
				ReadProcessMemory(phandle, (void*)(objAddress + 0x382 + 0x14 * index), &x2, sizeof(x2), 0);
				ReadProcessMemory(phandle, (void*)(objAddress + 0x384 + 0x14 * index), &y1, sizeof(y1), 0);
				ReadProcessMemory(phandle, (void*)(objAddress + 0x386 + 0x14 * index), &y2, sizeof(y2), 0);
				float fx1, fx2, fy1, fy2;
				fx1 = ((x1 - cameraPosX) * cameraZoom + 960) * resolutionX / 1920;
				fx2 = ((x2 - cameraPosX) * cameraZoom + 960) * resolutionX / 1920;
				fy1 = ((cameraPosY - y1 - 140) * cameraZoom + 540 + cameraZoom*64 * (cameraZoom <= 0.42 ? 0 : (cameraZoom - 0.42))) * (resolutionX * 0.5625) / 1080 + (resolutionY - resolutionX * 0.5625) / 2;
				fy2 = ((cameraPosY - y2 - 140) * cameraZoom + 540 + cameraZoom*64 * (cameraZoom <= 0.42 ? 0 : (cameraZoom - 0.42))) * (resolutionX * 0.5625) / 1080 + (resolutionY - resolutionX * 0.5625) / 2;
				//fy1 = ((cameraPosY - y1) * cameraZoom + 437) * (resolutionX * 0.5625) / 1080 + (resolutionY - resolutionX * 0.5625) / 2;
				//fy2 = ((cameraPosY - y2) * cameraZoom + 437) * (resolutionX * 0.5625) / 1080 + (resolutionY - resolutionX * 0.5625) / 2;
				if (type == 4) {
					DrawRectangle(pDevice, fx1, fx2, fy1, fy2,0, D3DCOLOR_ARGB(96, 255, 30, 30), D3DCOLOR_ARGB(255, 255, 0, 0));
				}
				else if (type == 7) {
					DrawRectangle(pDevice, fx1, fx2, fy1, fy2,0, D3DCOLOR_ARGB(96, 255, 255, 0), D3DCOLOR_ARGB(255, 255, 255, 0));
				}
				else {
					DrawRectangle(pDevice, fx1, fx2, fy1, fy2,0, D3DCOLOR_ARGB(96, 30, 30, 255), D3DCOLOR_ARGB(255, 0, 0, 255));
				}
			}
			it2 = boxMap.begin();
			int index = it2->first;
			int type = it2->second;
			//WORD x1, x2, y1, y2;
			signed short  x1, x2, y1, y2;
			ReadProcessMemory(phandle, (void*)(objAddress + 0x380 + 0x14 * index), &x1, sizeof(x1), 0);
			ReadProcessMemory(phandle, (void*)(objAddress + 0x382 + 0x14 * index), &x2, sizeof(x2), 0);
			ReadProcessMemory(phandle, (void*)(objAddress + 0x384 + 0x14 * index), &y1, sizeof(y1), 0);
			ReadProcessMemory(phandle, (void*)(objAddress + 0x386 + 0x14 * index), &y2, sizeof(y2), 0);
			float fx1, fx2, fy1, fy2;
			fx1 = ((x1 - cameraPosX) * cameraZoom + 960) * resolutionX / 1920;
			fx2 = ((x2 - cameraPosX) * cameraZoom + 960) * resolutionX / 1920;
			fy1 = ((cameraPosY - y1 - 140) * cameraZoom + 540 + cameraZoom*64 * (cameraZoom <= 0.42 ? 0 : (cameraZoom - 0.42))) * (resolutionX * 0.5625) / 1080 + (resolutionY - resolutionX * 0.5625) / 2;
			fy2 = ((cameraPosY - y2 - 140) * cameraZoom + 540 + cameraZoom*64 * (cameraZoom <= 0.42 ? 0 : (cameraZoom - 0.42))) * (resolutionX * 0.5625) / 1080 + (resolutionY - resolutionX * 0.5625) / 2;
			//fy1 = ((cameraPosY - y1-140) * cameraZoom + 540 + 64*(cameraZoom <= 0.42?0:(cameraZoom-0.42))) * (resolutionX * 0.5625) / 1080 + (resolutionY - resolutionX * 0.5625) / 2;
			//fy2 = ((cameraPosY - y2-140) * cameraZoom + 540 + 64*(cameraZoom <= 0.42 ? 0 : (cameraZoom - 0.42))) * (resolutionX * 0.5625) / 1080 + (resolutionY - resolutionX * 0.5625) / 2;
			if (type == 4) {
				DrawRectangle(pDevice, fx1, fx2, fy1, fy2, 0, D3DCOLOR_ARGB(96, 255, 30, 30), D3DCOLOR_ARGB(255, 255, 0, 0));
			}
			else if (type == 7) {
				DrawRectangle(pDevice, fx1, fx2, fy1, fy2, 0, D3DCOLOR_ARGB(96, 255, 255, 0), D3DCOLOR_ARGB(255, 255, 255, 0));
			}
			else {
				DrawRectangle(pDevice, fx1, fx2, fy1, fy2, 0, D3DCOLOR_ARGB(96, 30, 30, 255), D3DCOLOR_ARGB(255, 0, 0, 255));
			}
			boxMap.clear();
		}
		objMap.clear();

		inBattle = false;
	}
	return oEndScene(pDevice);
}
void _stdcall Hooked_BattleCamera() {
	inBattle = true;
	oBattleCamera();
}
int _cdecl Hooked_GetHitboxType(DWORD pt1, DWORD pt2) {
	int ret = oGetHitboxType(pt1, pt2);
	DWORD pt;
	if (pt1 > pt2) {
		pt = pt1 - 0x1C;
	}
	else {
		pt = pt2 - 0x1C;
	}
	DWORD objAddress;
	DWORD index;
	if (ret == 4) {
		ReadProcessMemory(phandle, (void*)(pt - 0x44), &objAddress, sizeof(objAddress), 0);
		ReadProcessMemory(phandle, (void*)(pt - 0xBC), &index, sizeof(index), 0);
	}
	else {
		ReadProcessMemory(phandle, (void*)(pt + 0x14), &objAddress, sizeof(objAddress), 0);
		ReadProcessMemory(phandle, (void*)(pt - 0xB8), &index, sizeof(index), 0);
	}
	std::map<DWORD,std::map<int,int>>::iterator it;
	it = objMap.find(objAddress);
	if (it == objMap.end()) {
		std::map<int, int> newMap;
		newMap.insert(std::pair<int, int>{index,ret});
		objMap.insert(std::pair<DWORD, std::map<int, int>>{objAddress,newMap});
	}
	else {
		std::map<int, int>::iterator it2;
		std::map<int, int> boxMap = it->second;
		it2 = boxMap.find(index);
		if (it2 == boxMap.end()) {
			it->second.insert(std::pair<int, int>{index, ret});
		}
		else if(ret == 4 || ret == 7){
			it->second.erase(index);
			it->second.insert(std::pair<int, int>{index, ret});
		}
	}
	return ret;
}
DWORD WINAPI MainThread(LPVOID hModule)
{
	phandle = GetCurrentProcess();
	ReadProcessMemory(phandle, (void*)(base_address + add_address), &objStart, sizeof(objStart), 0);
	if (!phandle) {
		MessageBoxA(NULL, "phandle not found", "Error", MB_OK);
		exit(0);
	}
	{
		oBattleCamera = (f_BattleCamera)(base_address + 0x55DE0);
		DetourRestoreAfterWith();

		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		DetourAttach(&(LPVOID&)oBattleCamera, Hooked_BattleCamera);
		DetourTransactionCommit();
	}
	{
		oGetHitboxType = (f_GetHitboxType)(base_address + 0xAC4F0);
		DetourRestoreAfterWith();

		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		DetourAttach(&(LPVOID&)oGetHitboxType, Hooked_GetHitboxType);
		DetourTransactionCommit();
	}
	Sleep(1000);
	
	if (GetD3D9Device(d3d9Device, sizeof(d3d9Device)))
	{
		std::cout << "[+] Found DirectXDevice vTable at: ";
		std::cout << std::hex << d3d9Device[0] << "\n";

		std::cout << "[+] Trying to hook function..\n";
		{
			// Using detours
			oEndScene = (f_EndScene)d3d9Device[42];

			DetourRestoreAfterWith();

			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());

			DetourAttach(&(LPVOID&)oEndScene, Hooked_EndScene);
			DetourTransactionCommit();
			
		}
		
	}
	else {
		MessageBoxA(NULL, "Error in getting D3D9 device", "Error", MB_ICONERROR);
	}
	
	return false;

}



BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		CreateThread(0, 0, MainThread, hModule, 0, 0);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
