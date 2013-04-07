//Developed by Viacheslav Avramenko
//KeyDisplayer

#include "stdafx.h"
#include "KeyWin.h"
#include <wchar.h>
#include <stdio.h>
#include <list>

using namespace std;

list<DWORD> down_vkCodes;


#define MAX_LOADSTRING 100


NOTIFYICONDATA	niData;	// notify icon data
#define TRAYICONID	1	//ID number for the Notify Icon

#define SWM_TRAYMSG	WM_APP	
#define SWM_EXIT	WM_APP + 1  //close the window

bool key_keepvis=true; //Keep window visible

HINSTANCE hInst;								
TCHAR szTitle[MAX_LOADSTRING]=_T("KeyWin");					
TCHAR szWindowClass[MAX_LOADSTRING]=_T("KEYLOGGER");			
HWND global_hWnd=NULL;

Bitmap_Operations *biop;
TCHAR * buf=new TCHAR[1001]();
TCHAR * buf_debug=new TCHAR[1001]();
size_t wn=0;
float rectBottom=0;
float textY=-40;

HHOOK hkb=NULL;
int edges_diameter=20;
int growth =0;
int addition =0;
RECT rect;
//func prototypes
void Draw_With_Buffering();
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
void ShowContextMenu(HWND hWnd);
__declspec(dllexport) LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
BOOL InstallHook();
BOOL UnHook();

//hack to make window always visible
void ForceForegroundWindow(HWND hWnd)
{
    DWORD  foreThread = GetWindowThreadProcessId(GetForegroundWindow(), 
        NULL);
    DWORD  appThread = GetCurrentThreadId();

    if (foreThread != appThread)
    {
        AttachThreadInput(foreThread, appThread, TRUE);
	SetWindowPos(hWnd, HWND_TOP, 10, 0, 110, 60, SWP_NOACTIVATE | SWP_NOMOVE | SWP_SHOWWINDOW);        
        ShowWindow(hWnd, SW_SHOW);
        AttachThreadInput(foreThread, appThread, FALSE);
    }
    else
    {
        ShowWindow(hWnd, SW_SHOW);
    }
	
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;
	MyRegisterClass(hInstance);
	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
	}

	return (int) msg.wParam;
}


ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_ICON1);
	wcex.hCursor		= NULL;
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_ICON1);

	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   

   hInst = hInstance; 
   global_hWnd = CreateWindow(szWindowClass, szTitle, WS_POPUP|WS_VISIBLE|WS_SYSMENU,
      10, 0, 110, 60, NULL, NULL, hInstance, NULL);

   if (!global_hWnd)
   {
      return FALSE;
   }

   SetWindowLong(global_hWnd, GWL_EXSTYLE, 
        GetWindowLong(global_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED  );

   SetLayeredWindowAttributes(global_hWnd, RGB(255, 255, 255), 
				0, LWA_COLORKEY);

   InstallHook();

   ShowWindow(global_hWnd, nCmdShow);
   UpdateWindow(global_hWnd);

	ZeroMemory(&niData,sizeof(NOTIFYICONDATA));

	niData.cbSize = sizeof(NOTIFYICONDATA);

	niData.uID = TRAYICONID;

	niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;

	// load the icon
	niData.hIcon = (HICON)LoadImage(hInstance,MAKEINTRESOURCE(IDI_ICON1),
		IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),
		LR_DEFAULTCOLOR);

	// the window to send messages to and the message to send
	niData.hWnd = global_hWnd;
    niData.uCallbackMessage = SWM_TRAYMSG;

	// tooltip message
    lstrcpyn(niData.szTip, _T("KeyWin"), sizeof(niData.szTip)/sizeof(TCHAR));

	Shell_NotifyIcon(NIM_ADD,&niData);

	// free icon handle
	if(niData.hIcon && DestroyIcon(niData.hIcon))
		niData.hIcon = NULL;


   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message) 
	{
	
	case WM_CREATE:
		{//Code to create instance of class and initialize buffers
         
		  biop = new Bitmap_Operations();
		   
		  GetClientRect(hWnd,&rect);

		  addition = 2;

		  biop->Initialize_Buffers(hWnd,1);
		   
		  biop->Create_Buffer(0);//(B)
		}
		break;
	//will be called periodically, if SetTimer function is invoked.
	case WM_TIMER:
		{

		if (key_keepvis) addition = 2;

		  //controls growth of the ellipse
		if (growth >= rect.bottom){
			addition = (key_keepvis)?0:-2;
		}

		if(growth<0) {
			  KillTimer(hWnd,1);
			  addition = 2;
			  growth=0;
		}else{
			  growth += addition;
			  Draw_With_Buffering();
			  ForceForegroundWindow(hWnd);
		 }
		 
		}
        break;

	case SWM_TRAYMSG:
		switch(lParam)
		{
		case WM_LBUTTONDBLCLK:
			ShowWindow(hWnd, SW_RESTORE);
			break;
		case WM_RBUTTONDOWN:
		case WM_CONTEXTMENU:
			ShowContextMenu(hWnd);
		}
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam); 

		switch (wmId)
		{
		case SWM_EXIT:
			DestroyWindow(hWnd);
			break;
		}
		return 1;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		KillTimer(hWnd,1);
				niData.uFlags = 0;
		Shell_NotifyIcon(NIM_DELETE,&niData);
		//Release Memory of Buffer
		 biop->Free_Buffer(0);
		 delete biop;
		 UnHook();
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void ShowContextMenu(HWND hWnd)
{
	POINT pt;
	GetCursorPos(&pt);
	HMENU hMenu = CreatePopupMenu();
	if(hMenu)
	{

		InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_EXIT, _T("Exit"));

		SetForegroundWindow(hWnd);

		TrackPopupMenu(hMenu, TPM_BOTTOMALIGN,
			pt.x, pt.y, 0, hWnd, NULL );
		DestroyMenu(hMenu);
	}
}

void Draw_With_Buffering()
{
	      
          HBRUSH brush = CreateSolidBrush(RGB(200,170,20));
		  HBRUSH background_brush = CreateSolidBrush(RGB(255,255,255));
          
		  SelectObject(biop->Get_DC_Buffer(0),brush);
		  FillRect(biop->Get_DC_Buffer(0),&rect,background_brush);
		  RoundRect(biop->Get_DC_Buffer(0),/*left*/ rect.left ,/*top*/ rect.top-edges_diameter
			  ,/*right*/ rect.right,/*bottom*/ growth, edges_diameter,edges_diameter);

		  SetBkMode(biop->Get_DC_Buffer(0), TRANSPARENT);

		  TextOut( biop->Get_DC_Buffer(0), 10//rect.right/2-wn*3 for middle
			  , growth+(int)(textY), (LPCTSTR)buf
			 , wn );

          biop->Copy_to_Screen(0);

		  //realease memory of the brushes
		  DeleteObject(background_brush);
		  DeleteObject(brush);
}


//--------------------------
BOOL InstallHook()
{
	HINSTANCE  hExe = GetModuleHandle(NULL);
    hkb=SetWindowsHookEx(WH_KEYBOARD_LL,(HOOKPROC)KeyboardProc,hExe,0);

    return TRUE;
}

BOOL UnHook()
{
    	
     BOOL unhooked = UnhookWindowsHookEx(hkb);
     return unhooked;
} 

VOID keypressed(TCHAR* data)
{
	_tcscpy(buf,data);
	wn=_tcslen(buf);
	SetTimer(global_hWnd,1,15,0);
}

bool isCapsLock()
{
     if ((GetKeyState(VK_CAPITAL) & 0x0001)!=0)
        return true;
     else
        return false;    
}

bool logicalXOR(bool p, bool q)
{
     /* since there is no operator for logical xor in c++
        it must be written in once of the equivelant forms */
     return ((p || q) && !(p && q));
}

__declspec(dllexport) LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    // Declare pointer to the KBDLLHOOKSTRUCT
    KBDLLHOOKSTRUCT *pKeyBoard = (KBDLLHOOKSTRUCT *)lParam;
        // Assign keyboard code to local variable
        DWORD vkCode = pKeyBoard->vkCode;
		list<DWORD>::iterator itDown_vkCodes;
		for(itDown_vkCodes=down_vkCodes.begin(); itDown_vkCodes!=down_vkCodes.end(); ++itDown_vkCodes)
			if((*itDown_vkCodes)==vkCode) break;
			

    switch (wParam)
    {
	case WM_KEYUP:
	{
			if(itDown_vkCodes!=down_vkCodes.end()){
				down_vkCodes.erase(itDown_vkCodes);
			}



		if(down_vkCodes.empty()) key_keepvis=false;

	}; break;
    case WM_KEYDOWN: 
    {
		if(itDown_vkCodes==down_vkCodes.end())
			down_vkCodes.push_back(vkCode);

           switch (vkCode) 
            {
            case VK_SPACE:
                keypressed(_T("SPACE"));
                break;
            case VK_RETURN:
                keypressed(_T("[ENTER]"));
                break;
            case VK_BACK:
                keypressed(_T("[BKSP]"));
                break;
            case VK_TAB:
                keypressed(_T("[TAB]"));
                break;
            case VK_LCONTROL:
            case VK_RCONTROL:
                keypressed(_T("[CTRL]"));
                break;
            case VK_LMENU:
            case VK_RMENU:
                keypressed(_T("[ALT]"));
                break;
            case VK_CAPITAL:
                keypressed(_T("[CAPS]"));
                break;
            case VK_ESCAPE:
                keypressed(_T("[ESC]"));
                break;
            case VK_INSERT:
                keypressed(_T("[INSERT]"));
                break;
            case VK_DELETE:
                keypressed(_T("[DEL]"));
                break;
            case VK_F1:
                keypressed(_T("[F1]"));
                break;
            case VK_F2:
                keypressed(_T("[F2]"));
                break;
            case VK_F3:
                keypressed(_T("[F3]"));
                break;				
            case VK_F4:
                keypressed(_T("[F4]"));
                break;
            case VK_F5:
                keypressed(_T("[F5]"));
                break;
            case VK_F6:
                keypressed(_T("[F6]"));
                break;
            case VK_F7:
                keypressed(_T("[F7]"));
                break;				
            case VK_F8:
                keypressed(_T("[F8]"));
                break;
            case VK_F9:
                keypressed(_T("[F9]"));
                break;
            case VK_F10:
                keypressed(_T("[F10]"));
                break;
            case VK_F11:
                keypressed(_T("[F11]"));
                break;				
            case VK_F12:
                keypressed(_T("[F12]"));
                break;				
            case VK_LSHIFT:
			case VK_RSHIFT:
                keypressed(_T("[SHIFT]"));
                break;	
           case VK_RIGHT:
                keypressed(_T("[RIGHT]"));
                break;
           case VK_LEFT:
                keypressed(_T("[LEFT]"));
                break;				
           case VK_UP:
                keypressed(_T("[UP]"));
                break;				
           case VK_DOWN:
                keypressed(_T("[DOWN]"));
                break;								
           case VK_PRIOR:
                keypressed(_T("[PGDWN]"));
                break;				
           case VK_NEXT:
                keypressed(_T("[PGUP]"));
                break;		
           case VK_SNAPSHOT:
                keypressed(_T("[PRNTSCR]"));
                break;								
           case VK_HOME:
                keypressed(_T("[HOME]"));
                break;				
           case VK_END:
                keypressed(_T("[END]"));
                break;					

				
				
            default: 
                            
				BYTE keyState[256];
				WCHAR buffer[16];

				GetKeyboardState((PBYTE)&keyState);
				ToUnicode(pKeyBoard->vkCode, pKeyBoard->scanCode, (PBYTE)&keyState, (LPWSTR)&buffer, sizeof(buffer) / 2, 0);
				keypressed(buffer);             
            }
 
		key_keepvis=true;
		}
    default:
        return CallNextHookEx( NULL, nCode, wParam, lParam );
    }

    return 0;


}