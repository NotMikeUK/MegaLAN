#include "stdafx.h"
#include <string>
#include <vector>
#include "commctrl.h"
#include "Windows.h"
#include <shellapi.h>
#include "MegaLAN.h"
#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

Login::Login()
{
	Username = L"";
	Password = L"";
}
void Login::Show()
{
	IsLoggingIn = false;
	DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_LOGIN), NULL, Login::LoginWndProc, (LPARAM)this);
	const LPCWSTR Software = L"Software";
	HKEY Key, MyKey;
	RegOpenKey(HKEY_CURRENT_USER, Software, &Key);
	RegCreateKeyEx(Key, L"MegaLAN", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &MyKey, NULL);
	RegSetKeyValue(MyKey, NULL, L"Username", REG_SZ, Username.c_str(), Username.length() * 2 + 2);
	if (IsAuthenticated)
		RegSetKeyValue(MyKey, NULL, L"Password", REG_SZ, Password.c_str(), Password.length() * 2 + 2);
	else
		RegSetKeyValue(MyKey, NULL, L"Password", REG_SZ, L"", 0);
	RegSetKeyValue(MyKey, NULL, L"Server", REG_SZ, Server.c_str(), Server.length() * 2 + 2);
	RegCloseKey(Key);
	RegCloseKey(MyKey);
}

INT_PTR CALLBACK Login::LoginWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		Login* Me = (Login*)lParam;
		if (Me)
		{
			SetWindowLongPtr(hDlg, DWLP_USER, lParam);
			Me->hDlg = hDlg;
		}
		const LPCWSTR Software = L"Software\\MegaLAN";
		HKEY Key;
		RegOpenKey(HKEY_CURRENT_USER, Software, &Key);
		WCHAR Username[255];
		memset(Username, 0, sizeof(Username));
		DWORD UsernameLen = sizeof(Username);
		if (RegGetValue(Key, NULL, L"Username", RRF_RT_REG_SZ, NULL, Username, &UsernameLen) != ERROR_SUCCESS)
			UsernameLen = 0;
		SendMessage(GetDlgItem(hDlg, IDC_USERNAME), WM_SETTEXT, 0, (LPARAM)&Username);
		Me->Username = Username;
		WCHAR Password[255];
		memset(Password, 0, sizeof(Password));
		DWORD PasswordLen = sizeof(Password);
		if (RegGetValue(Key, NULL, L"Password", RRF_RT_REG_SZ, NULL, Password, &PasswordLen) != ERROR_SUCCESS)
			PasswordLen = 0;
		SendMessage(GetDlgItem(hDlg, IDC_PASSWORD), WM_SETTEXT, 0, (LPARAM)&Password);
		Me->Password = Password;
		WCHAR Server[255];
		memset(Server, 0, sizeof(Server));
		DWORD ServerLen = sizeof(Server);
		if (RegGetValue(Key, NULL, L"Server", RRF_RT_REG_SZ, NULL, Server, &ServerLen) != ERROR_SUCCESS)
			ServerLen = 0;
		SendMessage(GetDlgItem(hDlg, IDC_SERVER), WM_SETTEXT, 0, (LPARAM)&Server);
		Me->Server = Server;
		RegCloseKey(Key);
		if (!Me->IsAuthenticated && UsernameLen > 2 && PasswordLen > 2 && ServerLen > 2)
		{
			PostMessage(hDlg, WM_COMMAND, IDC_LOGIN, 0);
		}
		Me->IsAuthenticated = false;
		if (ServerLen <= 2)
			SendMessage(GetDlgItem(hDlg, IDC_SERVER), WM_SETTEXT, 0, (LPARAM)L"MegaLAN.app");
	}
	break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_REGISTER)
		{
			SHELLEXECUTEINFO info = { 0 };
			info.cbSize = sizeof(info);
			info.hwnd = hDlg;
			WCHAR Server[255] = { 0 };
			SendMessage(GetDlgItem(hDlg, IDC_SERVER), WM_GETTEXT, 254, (LPARAM)Server);
			Socket.SendRawToServers(Server, (BYTE*)"URLAREGISTER", 12);
			SetTimer(hDlg, IDC_LOGIN, 2000, NULL);
			Login* Me = (Login*)GetWindowLongPtr(hDlg, DWLP_USER);
			Me->WaitingForURL = true;
		}
		if (LOWORD(wParam) == IDC_FORGOT_PASSWORD)
		{
			SHELLEXECUTEINFO info = { 0 };
			info.cbSize = sizeof(info);
			info.hwnd = hDlg;
			WCHAR Server[255] = { 0 };
			SendMessage(GetDlgItem(hDlg, IDC_SERVER), WM_GETTEXT, 254, (LPARAM)Server);
			Socket.SendRawToServers(Server, (BYTE*)"URLAFORGOTPW", 12);
			SetTimer(hDlg, IDC_LOGIN, 2000, NULL);
			Login* Me = (Login*)GetWindowLongPtr(hDlg, DWLP_USER);
			Me->WaitingForURL = true;
		}
		if (LOWORD(wParam) == IDC_LOGIN)
		{
			Login* Me = (Login*)GetWindowLongPtr(hDlg, DWLP_USER);
			WCHAR Username[255] = { 0 };
			SendMessage(GetDlgItem(hDlg, IDC_USERNAME), WM_GETTEXT, 254, (LPARAM)Username);
			Me->Username = Username;
			WCHAR Password[255] = { 0 };
			SendMessage(GetDlgItem(hDlg, IDC_PASSWORD), WM_GETTEXT, 254, (LPARAM)Password);
			Me->Password = Password;
			WCHAR Server[255] = { 0 };
			SendMessage(GetDlgItem(hDlg, IDC_SERVER), WM_GETTEXT, 254, (LPARAM)Server);
			Me->Server = Server;
			EnableWindow(GetDlgItem(hDlg, IDC_USERNAME), false);
			EnableWindow(GetDlgItem(hDlg, IDC_PASSWORD), false);
			EnableWindow(GetDlgItem(hDlg, IDC_SERVER), false);
			EnableWindow(GetDlgItem(hDlg, IDC_LOGIN), false);
			EnableWindow(GetDlgItem(hDlg, IDC_REGISTER), false);
			EnableWindow(GetDlgItem(hDlg, IDC_FORGOT_PASSWORD), false);
			Socket.SendLogin(Username, Password, Server);
			SetTimer(hDlg, IDC_LOGIN, 2000, NULL);
			Me->IsLoggingIn = true;
		}
		if (LOWORD(wParam) == IDCANCEL)
		{
			Login* Me = (Login*)GetWindowLongPtr(hDlg, DWLP_USER);
			Me->Password = L"";
			const LPCWSTR Software = L"Software\\MegaLAN";
			HKEY Key;
			RegOpenKey(HKEY_CURRENT_USER, Software, &Key);
			RegSetKeyValue(Key, NULL, L"Password", REG_SZ, L"", 2);
			RegCloseKey(Key);
			PostQuitMessage(0);
		}
		break;
	case WM_TIMER:
		if (wParam == IDC_LOGIN)
		{
			Login* Me = (Login*)GetWindowLongPtr(hDlg, DWLP_USER);
			EnableWindow(GetDlgItem(hDlg, IDC_USERNAME), true);
			EnableWindow(GetDlgItem(hDlg, IDC_PASSWORD), true);
			EnableWindow(GetDlgItem(hDlg, IDC_SERVER), true);
			EnableWindow(GetDlgItem(hDlg, IDC_LOGIN), true);
			EnableWindow(GetDlgItem(hDlg, IDC_REGISTER), true);
			EnableWindow(GetDlgItem(hDlg, IDC_FORGOT_PASSWORD), true);
			KillTimer(hDlg, IDC_LOGIN);
			MessageBox(NULL, L"Timed out contacting server.", L"Network Error", 0);
			if (Me)
			{
				Me->IsLoggingIn = false;
				Me->WaitingForURL = false;
			}
		}
	break;
	case WM_CLOSE:
		Login* Me = (Login*)GetWindowLongPtr(hDlg, DWLP_USER);
		if (Me)
			Me->hDlg = NULL;
		if (!Me->IsAuthenticated)
			PostQuitMessage(0);
		EndDialog(hDlg, 0);
		break;
	}
	return (INT_PTR)FALSE;
}
void Login::RecvPacket(struct InboundUDP &Packet)
{
	EnableWindow(GetDlgItem(hDlg, IDC_USERNAME), true);
	EnableWindow(GetDlgItem(hDlg, IDC_PASSWORD), true);
	EnableWindow(GetDlgItem(hDlg, IDC_SERVER), true);
	EnableWindow(GetDlgItem(hDlg, IDC_LOGIN), true);
	EnableWindow(GetDlgItem(hDlg, IDC_REGISTER), true);
	EnableWindow(GetDlgItem(hDlg, IDC_FORGOT_PASSWORD), true);
	KillTimer(hDlg, IDC_LOGIN);
	printf("Login recv %d\n", Packet.len);
	if (WaitingForURL && memcmp(Packet.buffer, "URLA", 4) == 0)
	{
		WaitingForURL = false;
		std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
		std::wstring URL = utf8_conv.from_bytes((char*)Packet.buffer + 4);
		ShellExecute(hWnd, L"open", URL.c_str(), 0, 0, 0);
	}
	if (IsLoggingIn && Packet.len >= 2 && memcmp(Packet.buffer, "OK", 2) == 0)
	{
		IsLoggingIn = false;
		IsAuthenticated = true;
		Socket.SetServer(&Packet.addr);
		EndDialog(hDlg, 1);
	}
	else if (IsLoggingIn)
	{
		IsLoggingIn = false;
		MessageBox(hDlg, L"Username/Password incorrect.", L"Login Failed.", 0);
	}
}
Login::~Login()
{
}
