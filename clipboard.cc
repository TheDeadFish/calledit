#include <stdshit.h>

char* clipBoad_GetText(void)
{
	while(!OpenClipboard(NULL)) Sleep(100);
	HANDLE data = GetClipboardData(CF_TEXT);
	char* str = xstrdup((char*)data);
	CloseClipboard();
	return str;
}

void clipBoad_SetText(const char* str)
{
	if(str == NULL)	return;
	const size_t len = strlen(str) + 1;
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
	memcpy(GlobalLock(hMem), str, len);
	GlobalUnlock(hMem);
	while(!OpenClipboard(0)) Sleep(100);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hMem);
	CloseClipboard();
}

