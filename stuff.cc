#include <stdshit.h>
#include <win32hlp.h>

void EnableDlgItems2(HWND hwnd, 
	int first, int count, BOOL bEnable)
{
	for(int i = 0; i < count; i++) {
		EnableDlgItem(hwnd, i+first, bEnable); }
}

u32 dlgChk_getBits(HWND hwnd, int first, int count) {
	u32 bits = 0;
	for(int i = 0; i < count; i++) {
		if(IsDlgButtonChecked(hwnd, first+i))
			_BSET(bits, i); }
	return bits;
}

void dlgChk_setBits(HWND hwnd, int first, int count, u32 bits) {
	for(int i = 0; i < count; i++) {
			dlgButton_setCheck(hwnd, first+i, bits & (1<<i)); }
}
