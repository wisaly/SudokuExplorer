/*-------------------------------
   Kristin's Sudoku Explorer
     Copyright (c) Kristin
   Start at 2010-3-31 20:46
-------------------------------*/
#include <windows.h>
#include <process.h>
#include "resource.h"

//#define DEBUG

#define X_WINDOW		775
#define Y_WINDOW		550
#define X_OUTAREA		500
#define Y_OUTAREA		325

#define STATUS_INPUT	0
#define STATUS_SOLVING	1
#define STATUS_CANCEL	2

#define ID_TIMER		3

#define WM_USER_READY	WM_USER+1
#define WM_USER_DONE	WM_USER+2
#define WM_USER_FAIL	WM_USER+3
#define WM_USER_CANCEL	WM_USER+4
#define WM_USER_CRASH	WM_USER+5

typedef struct 
{
	HWND hwnd;
	DWORD dwBlank;
} PARAMS,*PPARAMS;

typedef struct tagNODE
{
	DWORD x;			/* = i */
	DWORD y;			/* = j */
	DWORD o;			/* = j/3*3+i/3 */
} NODE,*PNODE;

DWORD	dwScr[9][9];	/* Use for screen */
DWORD	dwMap[9][9];	/* Use for solve */
DWORD	dwOri[9][9];	/* Use for save original status */
DWORD	dwStatus;
DWORD	cBlank;
BOOL	fSelect=FALSE;
DWORD	xSel,ySel;
char	szNums[]=" 123456789-";
PARAMS	ThreadParams;
char	szOutArea[10][50];
BOOL	hHash[9][10];
BOOL	vHash[9][10];
BOOL	oHash[9][10];
NODE	nList[82];		/* nList[0] is never used */

LRESULT	CALLBACK	WndProc(HWND,UINT,WPARAM,LPARAM);
BOOL CALLBACK		AboutDlgProc(HWND,UINT,WPARAM,LPARAM);
VOID				SolveThread(PVOID p);
void				HashInit(void);
int					KSudokuHash(DWORD d);
//int					KSudokuSolveDFS(DWORD d);
int					GetRect(int xNum,int yNum,PRECT prect);
void				OutAreaDraw(HDC hdc,HPEN hpen,HFONT hfont1,HFONT hfont2,PRECT rect);
void				OutAreaAdd(char *szAdd);
void				OutAreaClear(void);
BOOL				CheckCrash(void);
void				NumpadDraw(HDC hdc,HPEN hpen,HBRUSH hbr1,HBRUSH hbr2,int x,int y,PRECT prect);
void				NumpadClear(HDC hdc,HBRUSH hbr,PRECT prect);
int					NumpadRect(int n,PRECT prect);
BOOL				NumpadHtTest(WORD x,WORD y,WORD *pxCho,WORD *pyCho);

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,PSTR szCmdLine,int iCmdShow)
{
	static char	szAppName[]="KSudokuExplorer";
	static char szTitle[]="Sudoku Explorer 1.0";
	HWND		hwnd;
	MSG			msg;
	WNDCLASS	wc;

	wc.style		= 0;
	wc.cbClsExtra	= 0;
	wc.cbWndExtra	= 0;
	wc.lpfnWndProc	= WndProc;
	wc.hInstance	= hInstance;
	wc.hIcon		= LoadIcon(hInstance,MAKEINTRESOURCE(IDI_ICON));
	wc.hCursor		= LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground= (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName	= szAppName;
	wc.lpszClassName= szAppName;

	RegisterClass(&wc);
	hwnd=CreateWindow(szAppName,szTitle,WS_BORDER|WS_CAPTION|WS_MINIMIZEBOX|WS_SYSMENU,
					CW_USEDEFAULT,CW_USEDEFAULT,X_WINDOW,Y_WINDOW, /* 50*9=450 */
					NULL,NULL,hInstance,NULL);
	ShowWindow(hwnd,iCmdShow);
	UpdateWindow(hwnd);

	while(GetMessage(&msg,NULL,0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	static HINSTANCE hInst;
	static HDC		hdcMem;
	static HBITMAP	hBm;
	static HPEN		hpenThick,hpenThin,hPenFrame,hpenGrayFrame,hpenRed;
	static HBRUSH	hbrWhite,hbrGray;
	static HFONT	hfontNum,hfontBt;
	static LOGFONT	lf;
	static char		szFontName[]="Arial";
	static char		szPopupMenu[]="KSEPopup";
	static HWND		hBtStart,hBtClear,hBtCancel,hbtExit;
	static char		szBuffer[100];
	static HMENU	hMenuPopup;
	static UINT		uOldMenuSel;
	HBRUSH			hbrUse;
	PAINTSTRUCT		ps;
	HDC				hdc;
	unsigned int	i,j;
	WORD			xPos,yPos,xNum,yNum;
	POINT			pt;
	RECT			rect;

	switch(message)
	{
	case WM_CREATE:
		hInst			=((LPCREATESTRUCT)lParam)->hInstance;
		hpenThick		=CreatePen(PS_SOLID,5,RGB(0,0,0));
		hpenThin		=CreatePen(PS_SOLID,3,RGB(100,100,100));
		hPenFrame		=CreatePen(PS_INSIDEFRAME,4,RGB(0,128,255));
		hpenGrayFrame	=CreatePen(PS_INSIDEFRAME,3,RGB(50,50,50));
		hbrWhite		=GetStockObject(WHITE_BRUSH);
		hbrGray			=CreateSolidBrush(RGB(200,200,200));
		lf.lfHeight		=40;
		lf.lfWeight		=0;
		lf.lfEscapement	=0;
		lf.lfOrientation=0;
		lf.lfWeight		=FW_THIN;
		lf.lfItalic		=0;
		lf.lfUnderline	=0;
		lf.lfStrikeOut	=0;
		lf.lfCharSet	=DEFAULT_CHARSET;
		lf.lfOutPrecision=0;
		lf.lfClipPrecision=0;
		lf.lfQuality	=0;
		lf.lfPitchAndFamily=DEFAULT_PITCH;
		lstrcpy(lf.lfFaceName,szFontName);
		hfontNum=CreateFontIndirect(&lf);
		hfontBt=GetStockObject(DEFAULT_GUI_FONT);
		dwStatus=STATUS_INPUT;
		
		hBtStart=CreateWindow("button","搜索",WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
								500,30,60,30,hwnd,(HMENU)ID_START,
								hInst,NULL);
		SendMessage(hBtStart,WM_SETFONT,(WPARAM)hfontBt,(LPARAM)TRUE);
		hBtCancel=CreateWindow("button","取消",WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
								580,30,60,30,hwnd,(HMENU)ID_CANCEL,
								hInst,NULL);
		SendMessage(hBtCancel,WM_SETFONT,(WPARAM)hfontBt,(LPARAM)TRUE);
		hBtClear=CreateWindow("button","清除",WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
								500,70,60,30,hwnd,(HMENU)ID_CLEAR,
								hInst,NULL);
		SendMessage(hBtClear,WM_SETFONT,(WPARAM)hfontBt,(LPARAM)TRUE);
		hbtExit=CreateWindow("button","退出",WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
								580,70,60,30,hwnd,(HMENU)ID_EXIT,
								hInst,NULL);
		SendMessage(hbtExit,WM_SETFONT,(WPARAM)hfontBt,(LPARAM)TRUE);

		EnableWindow(hBtCancel,FALSE);

		hMenuPopup=LoadMenu(hInst,szPopupMenu);
		hMenuPopup=GetSubMenu(hMenuPopup,0);
		uOldMenuSel=ID_NUM0;

		hdc=GetDC(hwnd);
		hdcMem=CreateCompatibleDC(hdc);
		hBm=CreateCompatibleBitmap(hdc,X_WINDOW,Y_WINDOW);
		SelectObject(hdcMem,hBm);
		SelectObject(hdcMem,hfontNum);
		ReleaseDC(hwnd,hdc);

		
		SetBkMode(hdcMem,OPAQUE);
		SetBkColor(hdcMem,RGB(255,255,255));
		Rectangle(hdcMem,0,0,X_WINDOW,Y_WINDOW);
		SetBkMode(hdcMem,TRANSPARENT);
		
		/****** Draw Frame ******/
		SelectObject(hdcMem,hpenThick);
		Rectangle(hdcMem,0+25,0+25,450+25,450+25);
		/****** Draw thin Line ******/
		SelectObject(hdcMem,hpenThin);
		MoveToEx(hdcMem,  0+25, 50+25,NULL);
		LineTo  (hdcMem,450+25, 50+25);
		MoveToEx(hdcMem,  0+25,100+25,NULL);
		LineTo  (hdcMem,450+25,100+25);
		MoveToEx(hdcMem,  0+25,200+25,NULL);
		LineTo  (hdcMem,450+25,200+25);
		MoveToEx(hdcMem,  0+25,250+25,NULL);
		LineTo  (hdcMem,450+25,250+25);
		MoveToEx(hdcMem,  0+25,350+25,NULL);
		LineTo  (hdcMem,450+25,350+25);
		MoveToEx(hdcMem,  0+25,400+25,NULL);
		LineTo  (hdcMem,450+25,400+25);
		
		MoveToEx(hdcMem, 50+25,  0+25,NULL);
		LineTo  (hdcMem, 50+25,450+25);
		MoveToEx(hdcMem,100+25,  0+25,NULL);
		LineTo  (hdcMem,100+25,450+25);
		MoveToEx(hdcMem,200+25,  0+25,NULL);
		LineTo  (hdcMem,200+25,450+25);
		MoveToEx(hdcMem,250+25,  0+25,NULL);
		LineTo  (hdcMem,250+25,450+25);
		MoveToEx(hdcMem,350+25,  0+25,NULL);
		LineTo  (hdcMem,350+25,450+25);
		MoveToEx(hdcMem,400+25,  0+25,NULL);
		LineTo  (hdcMem,400+25,450+25);
		/****** Draw # line ******/
		SelectObject(hdcMem,hpenThick);
		MoveToEx(hdcMem,  0+25,150+25,NULL);
		LineTo  (hdcMem,450+25,150+25);
		MoveToEx(hdcMem,  0+25,300+25,NULL);
		LineTo  (hdcMem,450+25,300+25);
		MoveToEx(hdcMem,150+25,  0+25,NULL);
		LineTo  (hdcMem,150+25,450+25);
		MoveToEx(hdcMem,300+25,  0+25,NULL);
		LineTo  (hdcMem,300+25,450+25);
		/****** Draw Frame Rectangle again ******/
		MoveToEx(hdcMem,  0+25,  0+25,NULL);
		LineTo  (hdcMem,  0+25,450+25);
		LineTo  (hdcMem,450+25,450+25);
		LineTo  (hdcMem,450+25,  0+25);
		LineTo  (hdcMem,  0+25,  0+25);
		
		OutAreaDraw(hdcMem,hpenGrayFrame,hfontBt,hfontNum,&rect);

		return 0;
	case WM_PAINT:
		hdc=BeginPaint(hwnd,&ps);
		BitBlt(hdc,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right,ps.rcPaint.bottom,
				hdcMem,ps.rcPaint.left,ps.rcPaint.top,SRCCOPY);
		
		EndPaint(hwnd,&ps);
		return 0;
	case WM_CHAR:
		if(dwStatus==STATUS_SOLVING)
			return 0;
		if(fSelect)
		{
			switch(wParam)
			{
			case '0':
				fSelect=FALSE;
				j=dwScr[ySel][xSel];
				dwOri[ySel][xSel]=dwScr[ySel][xSel]=0;
				GetRect(xSel,ySel,&rect);
				FillRect(hdcMem,&rect,hbrWhite);
				InvalidateRect(hwnd,&rect,TRUE);
				NumpadClear(hdcMem,hbrWhite,&rect);
				InvalidateRect(hwnd,&rect,TRUE);
				break;
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				dwOri[ySel][xSel]=dwScr[ySel][xSel]=wParam-'0';
				fSelect=FALSE;
				GetRect(xSel,ySel,&rect);
				hbrUse=(dwScr[ySel][xSel]) ? hbrGray : hbrWhite;
				FillRect(hdcMem,&rect,hbrUse);
				DrawText(hdcMem,&szNums[dwScr[ySel][xSel]],1,&rect,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
				InvalidateRect(hwnd,&rect,TRUE);
				NumpadClear(hdcMem,hbrWhite,&rect);
				InvalidateRect(hwnd,&rect,TRUE);
				break;
			}
		}
		/*
		else
		{
			switch(wParam)
			{
			case 't':
				break;
			}
		}*/
		return 0;
	case WM_KEYDOWN:
		if(dwStatus==STATUS_SOLVING)
			return 0;
		if(fSelect)
		{
			switch(wParam)
			{
			case VK_UP:
				if(ySel>0)
				{
					ySel--;
					GetRect(xSel,ySel,&rect);
					SelectObject(hdcMem,hPenFrame);
					hbrUse=(dwScr[ySel][xSel] && (dwScr[ySel][xSel]==dwOri[ySel][xSel]))? hbrGray : hbrWhite;
					SelectObject(hdcMem,hbrUse);
					Rectangle(hdcMem,rect.left,rect.top,rect.right,rect.bottom);
					SelectObject(hdcMem,hbrWhite);
					DrawText(hdcMem,&szNums[dwScr[ySel][xSel]],1,&rect,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
					InvalidateRect(hwnd,&rect,TRUE);
					GetRect(xSel,ySel+1,&rect);
					hbrUse=(dwOri[ySel+1][xSel] && (dwScr[ySel+1][xSel]==dwOri[ySel+1][xSel]))? hbrGray : hbrWhite;
					FillRect(hdcMem,&rect,hbrUse);
					DrawText(hdcMem,&szNums[dwScr[ySel+1][xSel]],1,&rect,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
					InvalidateRect(hwnd,&rect,TRUE);
					NumpadDraw(hdcMem,hpenGrayFrame,hbrWhite,hbrGray,xSel,ySel,&rect);
					InvalidateRect(hwnd,&rect,TRUE);
				}
				break;
			case VK_DOWN:
				if(ySel<8)
				{
					ySel++;
					GetRect(xSel,ySel,&rect);
					SelectObject(hdcMem,hPenFrame);
					hbrUse=(dwOri[ySel][xSel] && (dwScr[ySel][xSel]==dwOri[ySel][xSel]))? hbrGray : hbrWhite;
					SelectObject(hdcMem,hbrUse);
					Rectangle(hdcMem,rect.left,rect.top,rect.right,rect.bottom);
					SelectObject(hdcMem,hbrWhite);
					DrawText(hdcMem,&szNums[dwScr[ySel][xSel]],1,&rect,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
					InvalidateRect(hwnd,&rect,TRUE);
					GetRect(xSel,ySel-1,&rect);
					hbrUse=(dwOri[ySel-1][xSel] && (dwScr[ySel-1][xSel]==dwOri[ySel-1][xSel]))? hbrGray : hbrWhite;
					FillRect(hdcMem,&rect,hbrUse);
					DrawText(hdcMem,&szNums[dwScr[ySel-1][xSel]],1,&rect,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
					InvalidateRect(hwnd,&rect,TRUE);
					NumpadDraw(hdcMem,hpenGrayFrame,hbrWhite,hbrGray,xSel,ySel,&rect);
					InvalidateRect(hwnd,&rect,TRUE);
				}
				break;
			case VK_LEFT:
				if(xSel>0)
				{
					xSel--;
					GetRect(xSel,ySel,&rect);
					SelectObject(hdcMem,hPenFrame);
					hbrUse=(dwOri[ySel][xSel] && (dwScr[ySel][xSel]==dwOri[ySel][xSel]))? hbrGray : hbrWhite;
					SelectObject(hdcMem,hbrUse);
					Rectangle(hdcMem,rect.left,rect.top,rect.right,rect.bottom);
					SelectObject(hdcMem,hbrWhite);
					DrawText(hdcMem,&szNums[dwScr[ySel][xSel]],1,&rect,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
					InvalidateRect(hwnd,&rect,TRUE);
					GetRect(xSel+1,ySel,&rect);
					hbrUse=(dwOri[ySel][xSel+1] && (dwScr[ySel][xSel+1]==dwOri[ySel][xSel+1]))? hbrGray : hbrWhite;
					FillRect(hdcMem,&rect,hbrUse);
					DrawText(hdcMem,&szNums[dwScr[ySel][xSel+1]],1,&rect,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
					InvalidateRect(hwnd,&rect,TRUE);
					NumpadDraw(hdcMem,hpenGrayFrame,hbrWhite,hbrGray,xSel,ySel,&rect);
					InvalidateRect(hwnd,&rect,TRUE);
				}
				break;
			case VK_RIGHT:
				if(xSel<8)
				{
					xSel++;
					GetRect(xSel,ySel,&rect);
					SelectObject(hdcMem,hPenFrame);
					hbrUse=(dwOri[ySel][xSel] && (dwScr[ySel][xSel]==dwOri[ySel][xSel]))? hbrGray : hbrWhite;
					SelectObject(hdcMem,hbrUse);
					Rectangle(hdcMem,rect.left,rect.top,rect.right,rect.bottom);
					SelectObject(hdcMem,hbrWhite);
					DrawText(hdcMem,&szNums[dwScr[ySel][xSel]],1,&rect,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
					InvalidateRect(hwnd,&rect,TRUE);
					GetRect(xSel-1,ySel,&rect);
					hbrUse=(dwOri[ySel][xSel-1] && (dwScr[ySel][xSel-1]==dwOri[ySel][xSel-1]))? hbrGray : hbrWhite;
					FillRect(hdcMem,&rect,hbrUse);
					DrawText(hdcMem,&szNums[dwScr[ySel][xSel-1]],1,&rect,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
					InvalidateRect(hwnd,&rect,TRUE);
					NumpadDraw(hdcMem,hpenGrayFrame,hbrWhite,hbrGray,xSel,ySel,&rect);
					InvalidateRect(hwnd,&rect,TRUE);
				}
				break;
			default:
				return 0;
			}
		}
		else
		{
			switch(wParam)
			{
			case VK_UP:
			case VK_DOWN:
			case VK_LEFT:
			case VK_RIGHT:
				fSelect=TRUE;
				SendMessage(hwnd,message,wParam,lParam);
				return 0;
			default:
				return 0;
			}
		}
		return 0;
	case WM_LBUTTONUP:
		if(dwStatus==STATUS_SOLVING)
			return 0;
		xPos=LOWORD(lParam);
		yPos=HIWORD(lParam);
		if(NumpadHtTest(xPos,yPos,&xNum,&yNum)==TRUE)
		{
			SendMessage(hwnd,WM_CHAR,(WPARAM)('0'+xNum+yNum*5),0);
			return 0;
		}
		return 0;
	case WM_LBUTTONDOWN:
		if(dwStatus==STATUS_SOLVING)
			return 0;
		xPos=LOWORD(lParam);
		yPos=HIWORD(lParam);
		if((xPos<25)||(yPos<25))
			return 0;
		xNum=(xPos-25)/50;
		yNum=(yPos-25)/50;
		if(GetRect(xNum,yNum,&rect))    /* not in the blank */
			return 0;
		SetFocus(hwnd);
		if(fSelect)
		{
			if((xNum==xSel) && (yNum==ySel))
			{
				fSelect=FALSE;
				hbrUse=(dwOri[ySel][xSel] && (dwScr[ySel][xSel]==dwOri[ySel][xSel]))? hbrGray : hbrWhite;
				FillRect(hdcMem,&rect,hbrUse);
				DrawText(hdcMem,&szNums[dwScr[ySel][xSel]],1,&rect,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
				InvalidateRect(hwnd,&rect,TRUE);
				NumpadClear(hdcMem,hbrWhite,&rect);
				InvalidateRect(hwnd,&rect,TRUE);
			}
			else
			{
				SelectObject(hdcMem,hPenFrame);
				hbrUse=(dwOri[yNum][xNum] && (dwScr[yNum][xNum]==dwOri[yNum][xNum]))? hbrGray : hbrWhite;
				SelectObject(hdcMem,hbrUse);
				Rectangle(hdcMem,rect.left,rect.top,rect.right,rect.bottom);
				SelectObject(hdcMem,hbrWhite);
				DrawText(hdcMem,&szNums[dwScr[yNum][xNum]],1,&rect,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
				InvalidateRect(hwnd,&rect,TRUE);
				GetRect(xSel,ySel,&rect);
				hbrUse=(dwOri[ySel][xSel] && (dwScr[ySel][xSel]==dwOri[ySel][xSel]))? hbrGray : hbrWhite;
				FillRect(hdcMem,&rect,hbrUse);
				DrawText(hdcMem,&szNums[dwScr[ySel][xSel]],1,&rect,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
				InvalidateRect(hwnd,&rect,TRUE);
				xSel=xNum;
				ySel=yNum;
				NumpadDraw(hdcMem,hpenGrayFrame,hbrWhite,hbrGray,xSel,ySel,&rect);
				InvalidateRect(hwnd,&rect,TRUE);
			}
		}
		else
		{
			fSelect=TRUE;
			xSel=xNum;
			ySel=yNum;
			SelectObject(hdcMem,hPenFrame);
			hbrUse=(dwOri[ySel][xSel] && (dwScr[ySel][xSel]==dwOri[ySel][xSel]))? hbrGray : hbrWhite;
			SelectObject(hdcMem,hbrUse);
			Rectangle(hdcMem,rect.left,rect.top,rect.right,rect.bottom);
			SelectObject(hdcMem,hbrWhite);
			DrawText(hdcMem,&szNums[dwScr[yNum][xNum]],1,&rect,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
			InvalidateRect(hwnd,&rect,TRUE);
			NumpadDraw(hdcMem,hpenGrayFrame,hbrWhite,hbrGray,xSel,ySel,&rect);
			InvalidateRect(hwnd,&rect,TRUE);
		}
		return 0;
	case WM_RBUTTONUP:
		pt.x=LOWORD(lParam);
		pt.y=HIWORD(lParam);
		if((pt.x<25)||(pt.x>=475)||(pt.y<25)||(pt.y>=475))
			return 0;
		xNum=(WORD)(pt.x-25)/50;
		yNum=(WORD)(pt.y-25)/50;
		if(fSelect)
		{
			if((xNum==xSel)&&(yNum==ySel))
				;
			else
				SendMessage(hwnd,WM_LBUTTONDOWN,wParam,lParam);
		}
		else
			SendMessage(hwnd,WM_LBUTTONDOWN,wParam,lParam);
		CheckMenuItem(hMenuPopup,uOldMenuSel,MF_UNCHECKED);
		uOldMenuSel=ID_NUM0+dwScr[yNum][xNum];
		CheckMenuItem(hMenuPopup,uOldMenuSel,MF_CHECKED);
		ClientToScreen(hwnd,&pt);
		TrackPopupMenu(hMenuPopup,TPM_RIGHTBUTTON,pt.x,pt.y,0,hwnd,NULL);
		return 0;
	case WM_COMMAND:
		switch(HIWORD(wParam))
		{
		case BN_CLICKED:
			switch(LOWORD(wParam))
			{
			case ID_START:
				
				cBlank=0;
				for(j=0;j<9;j++)
					for(i=0;i<9;i++)
						if(dwScr[j][i])
						{
							dwMap[j][i]=dwScr[j][i];
							GetRect(i,j,&rect);
							FillRect(hdcMem,&rect,hbrGray);
							DrawText(hdcMem,&szNums[dwScr[j][i]],1,&rect,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
							InvalidateRect(hwnd,&rect,TRUE);
						}
				for(j=0;j<9;j++)
					for(i=0;i<9;i++)
						if(!dwMap[j][i])
						{
							cBlank++;
							nList[cBlank].x=i;
							nList[cBlank].y=j;
							nList[cBlank].o=j/3*3+i/3;
						}
				EnableWindow(hBtStart,FALSE);
				EnableWindow(hBtCancel,TRUE);
				EnableWindow(hBtClear,FALSE);
				ThreadParams.dwBlank=cBlank;
				ThreadParams.hwnd=hwnd;
				_beginthread(SolveThread,0,(PVOID)&ThreadParams);
				break;

			case ID_CLEAR:
				for(j=0;j<9;j++)
					for(i=0;i<9;i++)
					{
						dwOri[j][i]=dwMap[j][i]=dwScr[j][i]=0;
						GetRect(i,j,&rect);
						FillRect(hdcMem,&rect,hbrWhite);
					}
				for(j=0;j<9;j++)
					for(i=0;i<10;i++)
					{
						hHash[j][i]=vHash[j][i]=oHash[j][i]=FALSE;
					}
				InvalidateRect(hwnd,NULL,TRUE);
				UpdateWindow(hwnd);
				break;
			case ID_CANCEL:
				dwStatus=STATUS_CANCEL;
				break;
			case ID_EXIT:
				SendMessage(hwnd,WM_CLOSE,0,0);
				return 0;
			case ID_NUM0:
			case ID_NUM1:
			case ID_NUM2:
			case ID_NUM3:
			case ID_NUM4:
			case ID_NUM5:
			case ID_NUM6:
			case ID_NUM7:
			case ID_NUM8:
			case ID_NUM9:
				SendMessage(hwnd,WM_CHAR,LOWORD(wParam)-ID_NUM0+'0',0);
				break;
			case ID_ABOUT:
				DialogBox(hInst,"AboutBox",hwnd,AboutDlgProc);
				return 0;
			}
		}
		return 0;
	case WM_TIMER:
		for(j=0;j<9;j++)
			for(i=0;i<9;i++)
			{
				if(dwScr[j][i]!=dwMap[j][i])
				{
					dwScr[j][i]=dwMap[j][i];
					GetRect(i,j,&rect);
					FillRect(hdcMem,&rect,hbrWhite);
					DrawText(hdcMem,&szNums[dwScr[j][i]],1,&rect,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
					InvalidateRect(hwnd,&rect,TRUE);
				}
			}
		return 0;
	case WM_USER_READY:
		OutAreaAdd("准备完成");
		OutAreaDraw(hdcMem,hpenGrayFrame,hfontBt,hfontNum,&rect);
		InvalidateRect(hwnd,&rect,TRUE);
		return 0;
	case WM_USER_DONE:
		wsprintf(szBuffer,"搜索完成，用时%d毫秒。",(long)lParam);
		OutAreaAdd(szBuffer);
		OutAreaDraw(hdcMem,hpenGrayFrame,hfontBt,hfontNum,&rect);
		InvalidateRect(hwnd,&rect,TRUE);
		for(j=0;j<9;j++)
			for(i=0;i<9;i++)
				dwScr[j][i]=dwMap[j][i];
		for(j=0;j<9;j++)
			for(i=0;i<9;i++)
			{
				GetRect(i,j,&rect);
				hbrUse=(dwOri[j][i] && (dwScr[j][i]==dwOri[j][i]))? hbrGray : hbrWhite;
				FillRect(hdcMem,&rect,hbrUse);
				DrawText(hdcMem,&szNums[dwScr[j][i]],1,&rect,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
			}
		InvalidateRect(hwnd,NULL,TRUE);
		EnableWindow(hBtStart,TRUE);
		EnableWindow(hBtCancel,FALSE);
		EnableWindow(hBtClear,TRUE);
		return 0;
	case WM_USER_FAIL:
		wsprintf(szBuffer,"搜索失败，数独无解，用时%d毫秒。",(long)lParam);
		OutAreaAdd(szBuffer);
		OutAreaDraw(hdcMem,hpenGrayFrame,hfontBt,hfontNum,&rect);
		InvalidateRect(hwnd,&rect,TRUE);
		for(j=0;j<9;j++)
			for(i=0;i<9;i++)
				dwScr[j][i]=dwMap[j][i];
		for(j=0;j<9;j++)
			for(i=0;i<9;i++)
			{
				GetRect(i,j,&rect);
				hbrUse=(dwOri[j][i] && (dwScr[j][i]==dwOri[j][i]))? hbrGray : hbrWhite;
				FillRect(hdcMem,&rect,hbrUse);
				DrawText(hdcMem,&szNums[dwScr[j][i]],1,&rect,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
			}
		InvalidateRect(hwnd,NULL,TRUE);
		EnableWindow(hBtStart,TRUE);
		EnableWindow(hBtCancel,FALSE);
		EnableWindow(hBtClear,TRUE);
		return 0;
	case WM_USER_CANCEL:
		wsprintf(szBuffer,"搜索被取消，用时%d毫秒。",(long)lParam);
		OutAreaAdd(szBuffer);
		OutAreaDraw(hdcMem,hpenGrayFrame,hfontBt,hfontNum,&rect);
		InvalidateRect(hwnd,&rect,TRUE);
		for(j=0;j<9;j++)
			for(i=0;i<9;i++)
				dwScr[j][i]=dwMap[j][i];
		for(j=0;j<9;j++)
			for(i=0;i<9;i++)
			{
				GetRect(i,j,&rect);
				hbrUse=(dwOri[j][i] && (dwScr[j][i]==dwOri[j][i]))? hbrGray : hbrWhite;
				FillRect(hdcMem,&rect,hbrUse);
				DrawText(hdcMem,&szNums[dwScr[j][i]],1,&rect,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
			}
		InvalidateRect(hwnd,NULL,TRUE);
		EnableWindow(hBtStart,TRUE);
		EnableWindow(hBtCancel,FALSE);
		EnableWindow(hBtClear,TRUE);
		return 0;
	case WM_USER_CRASH:
		OutAreaAdd("搜索无法进行，请检查输入。");
		OutAreaDraw(hdcMem,hpenGrayFrame,hfontBt,hfontNum,&rect);
		InvalidateRect(hwnd,&rect,TRUE);
		for(j=0;j<9;j++)
			for(i=0;i<9;i++)
				dwScr[j][i]=dwMap[j][i];
		for(j=0;j<9;j++)
			for(i=0;i<9;i++)
			{
				GetRect(i,j,&rect);
				hbrUse=(dwOri[j][i] && (dwScr[j][i]==dwOri[j][i]))? hbrGray : hbrWhite;
				FillRect(hdcMem,&rect,hbrUse);
				DrawText(hdcMem,&szNums[dwScr[j][i]],1,&rect,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
			}
		InvalidateRect(hwnd,NULL,TRUE);
		EnableWindow(hBtStart,TRUE);
		EnableWindow(hBtCancel,FALSE);
		EnableWindow(hBtClear,TRUE);
		return 0;
	case WM_KILLFOCUS:
		fSelect=FALSE;
		GetRect(xSel,ySel,&rect);
		hbrUse=(dwOri[ySel][xSel] && (dwScr[ySel][xSel]==dwOri[ySel][xSel]))? hbrGray : hbrWhite;
		FillRect(hdcMem,&rect,hbrUse);
		DrawText(hdcMem,&szNums[dwScr[ySel][xSel]],1,&rect,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
		InvalidateRect(hwnd,&rect,TRUE);
		NumpadClear(hdcMem,hbrWhite,&rect);
		InvalidateRect(hwnd,&rect,TRUE);
		return 0;
	case WM_INITMENUPOPUP:
		EnableMenuItem((HMENU)wParam,ID_START,(dwStatus==STATUS_SOLVING)?MF_GRAYED:MF_ENABLED);
		EnableMenuItem((HMENU)wParam,ID_CLEAR,(dwStatus==STATUS_SOLVING)?MF_GRAYED:MF_ENABLED);
		EnableMenuItem((HMENU)wParam,ID_CANCEL,(dwStatus==STATUS_SOLVING)?MF_ENABLED:MF_GRAYED);
		return 0;
	case WM_DESTROY:
		ReleaseDC(hwnd,hdcMem);
		DeleteObject(hpenThick);
		DeleteObject(hpenThin);
		DeleteObject(hPenFrame);
		DeleteObject(hBm);
		DeleteObject(hbrGray);
		DeleteObject(hfontNum);
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd,message,wParam,lParam);
}
/* 
int KSudokuSolveDFS(DWORD d)
{
	DWORD f[10]={0},i,j,k,ti,tj;
	
	if(!d)					// Find result! 
		return 1;
	for(j=0;j<9;j++)
		for(i=0;i<9;i++)
			if(!dwMap[j][i])	// Find a blank to fill
			{
				ti=i/3*3;
				tj=j/3*3;

				for(k=0;k<9;k++)
				{
					f[dwMap[k][i]]=1;
					f[dwMap[j][k]]=1;
					f[dwMap[tj+k/3][ti+k%3]]=1;
				}// end of for k
				for(k=1;k<10;k++)
				{
					if(!f[k])
					{
						dwMap[j][i]=k;
						if(KSudokuSolveDFS(d-1))
							return 1;
						dwMap[j][i]=0;
					}// end of if(!f[k])
				}// end of for k 
			}// end of if(!s[j][i])
	return 0;	// cannot find
}// end of KSudokuSolve();
*/
int GetRect(int xNum,int yNum,PRECT prect) /* xNum & yNum from [0,8] */
{
	int tDel,bDel,lDel,rDel;

	if(!prect || (xNum<0) || (xNum>=9) || (yNum<0) || (yNum>=9))
		return 1;
	
	switch(xNum%3)
	{
	case 0:
		lDel=2;
		rDel=1;
		break;
	case 1:
		lDel=1;
		rDel=1;
		break;
	case 2:
		lDel=1;
		rDel=2;
		break;
	}
	switch(yNum%3)
	{
	case 0:
		tDel=2;
		bDel=1;
		break;
	case 1:
		tDel=1;
		bDel=1;
		break;
	case 2:
		tDel=1;
		bDel=2;
		break;
	}
	
	prect->top		= 25+yNum*50+tDel+1;
	prect->bottom	= 25+yNum*50-bDel+50;
	prect->left		= 25+xNum*50+lDel+1;
	prect->right	= 25+xNum*50-rDel+50;

	return 0;
}

VOID SolveThread(PVOID p)
{
	LONG lTime;
	PPARAMS pparams;
	int iResult;
	pparams=(PPARAMS)p;
	
	if(CheckCrash())
	{
		SendMessage(pparams->hwnd,WM_USER_CRASH,0,0);
		_endthread();
	}
	SetTimer(pparams->hwnd,ID_TIMER,100,NULL);
	dwStatus=STATUS_SOLVING;
	HashInit();
	SendMessage(pparams->hwnd,WM_USER_READY,0,0);
	lTime=GetCurrentTime();
	iResult=KSudokuHash(pparams->dwBlank);
	lTime=GetCurrentTime()-lTime;
	KillTimer(pparams->hwnd,ID_TIMER);
	switch(iResult)
	{
	case 0:
		SendMessage(pparams->hwnd,WM_USER_FAIL,0,lTime);
		break;
	case 1:
		SendMessage(pparams->hwnd,WM_USER_DONE,0,lTime);
		break;
	case 2:
		SendMessage(pparams->hwnd,WM_USER_CANCEL,0,lTime);
		break;
	}
	dwStatus=STATUS_INPUT;
	_endthread();
}

void OutAreaAdd(char *szAdd)
{
	int i;
	for(i=0;i<9;i++)
	{
		lstrcpy(szOutArea[i],szOutArea[i+1]);
	}
	lstrcpy(szOutArea[9],szAdd);
}

void OutAreaClear(void)
{
	int i;
	for(i=0;i<9;i++)
	{
		szOutArea[i][0]='\0';
	}
}

void OutAreaDraw(HDC hdc,HPEN hpen,HFONT hfont1,HFONT hfont2,PRECT rect)
{
	int i;
	SelectObject(hdc,hpen);
	SelectObject(hdc,hfont1);
	SetRect(rect,X_OUTAREA,Y_OUTAREA,X_OUTAREA+250,Y_OUTAREA+150);
	Rectangle(hdc,rect->left,rect->top,rect->right,rect->bottom);
	for(i=0;i<10;i++)
	{
		TextOut(hdc,X_OUTAREA+5,Y_OUTAREA+5+i*14,szOutArea[i],lstrlen(szOutArea[i]));
	}
	SelectObject(hdc,hfont2);
}

void HashInit(void)
{
	int i,j,n;
	
	/* Build Hash */
	for(j=0;j<9;j++)
		for(i=0;i<9;i++)
		{
			n=dwMap[j][i];
			hHash[j][n]=TRUE;
			vHash[i][n]=TRUE;
			oHash[j/3*3+i/3][n]=TRUE;
		}
}

/* Faster than DFS , return 0 when failed ,1 when success, 2 when user break */
int KSudokuHash(DWORD d)
{
	int		k;
	DWORD	dwRet;
	DWORD	xNow=nList[d].x;
	DWORD	yNow=nList[d].y;
	DWORD	oNow=nList[d].o;
	
#ifdef _DEBUG
	Sleep(10);
#endif

	if(dwStatus==STATUS_CANCEL)
		return 2;

	if(d==0)
		return 1;
	
	for(k=1;k<10;k++)
	{
		if(hHash[yNow][k]||vHash[xNow][k]||oHash[oNow][k])
			continue ;

		hHash[yNow][k]=vHash[xNow][k]=oHash[oNow][k]=TRUE;
		dwMap[yNow][xNow]=k;
		
		dwRet=KSudokuHash(d-1);
		if(dwRet)
			return dwRet;

		dwMap[yNow][xNow]=0;
		hHash[yNow][k]=vHash[xNow][k]=oHash[oNow][k]=FALSE;
	}

	return 0;
}

BOOL CheckCrash(void)
{
	int i,j,k,ti,tj;
	
	for(j=0;j<9;j++)
		for(i=0;i<9;i++)
			for(k=0;k<9;k++)
				if(dwOri[j][i])
				{
					ti=i/3*3;
					tj=j/3*3;
					
					if((k!=j)&&(dwOri[k][i]==dwOri[j][i]))
						return TRUE;
					if((k!=i)&&(dwOri[j][k]==dwOri[j][i]))
						return TRUE;
					if(((tj+k/3)!=j)&&((ti+k%3)!=i)&&(dwOri[tj+k/3][ti+k%3]==dwOri[j][i]))
						return TRUE;
				}
	return FALSE;
}

int NumpadRect(int n,PRECT prect)
{
	int tDel,bDel,lDel,rDel;

	if(!prect || (n<0) || (n>9))
		return 1;
	
	switch(n%5)
	{
	case 0:
		lDel=3;
		rDel=1;
		break;
	case 1:
	case 2:
	case 3:
		lDel=2;
		rDel=1;
		break;
	case 4:
		lDel=2;
		rDel=3;
		break;
	}
	switch(n/5)
	{
	case 0:
		tDel=3;
		bDel=1;
		break;
	case 1:
		tDel=2;
		bDel=3;
		break;
	}
	
	prect->top		= 200+(n/5)*50+tDel;
	prect->bottom	= 200+(n/5)*50-bDel+50;
	prect->left		= 500+(n%5)*50+lDel;
	prect->right	= 500+(n%5)*50-rDel+50;

	return 0;
}

void NumpadDraw(HDC hdc,HPEN hpen,HBRUSH hbr1,HBRUSH hbr2,int x,int y,PRECT prect)
{
	DWORD k;
	DWORD f[10]={0};

	SelectObject(hdc,hpen);
	Rectangle(hdc,500,200,750,300);
	MoveToEx (hdc,500,250,NULL);
	LineTo   (hdc,750,250);
	
	MoveToEx (hdc,550,202,NULL);
	LineTo   (hdc,550,298);
	MoveToEx (hdc,600,202,NULL);
	LineTo   (hdc,600,298);
	MoveToEx (hdc,650,202,NULL);
	LineTo   (hdc,650,298);
	MoveToEx (hdc,700,202,NULL);
	LineTo   (hdc,700,298);
	for(k=0;k<9;k++)
	{
		f[dwScr[k][x]]=1;
		f[dwScr[y][k]]=1;
		f[dwScr[y/3*3+k/3][x/3*3+k%3]]=1;
	}

	for(k=0;k<10;k++)
	{
		NumpadRect(k,prect);
		if(k==dwScr[y][x])
		{
			FillRect(hdc,prect,hbr2);
			DrawText(hdc,&szNums[k],1,prect,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
			continue;
		}
		if(f[k])
		{
			FillRect(hdc,prect,hbr1);
			SetTextColor(hdc,RGB(255,0,0));
			DrawText(hdc,&szNums[k],1,prect,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
			SetTextColor(hdc,RGB(0,0,0));
			continue;
		}
		else
		{
			DrawText(hdc,&szNums[k],1,prect,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
			continue;
		}
	}
	SetRect(prect,500,200,750,300);
}


void NumpadClear(HDC hdc,HBRUSH hbr,PRECT prect)
{
	SetRect(prect,500,200,750,300);
	FillRect(hdc,prect,hbr);
}

BOOL NumpadHtTest(WORD x,WORD y,WORD *pxCho,WORD *pyCho)
{
	if((x<500)||(x>=750)||(y<200)||(y>=300))
		return FALSE;
	x-=500;
	y-=200;
	*pxCho=x/50;
	*pyCho=y/50;
	return TRUE;
}

BOOL CALLBACK AboutDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		return 0;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			EndDialog(hDlg,0);
			return TRUE;
		}
		break;
	}
	return FALSE;
}