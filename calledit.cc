#include <stdshit.h>
#include <win32hlp.h>
#include "resource.h"
const char progName[] = "call edit";

char* clipBoad_GetText(void);
void clipBoad_SetText(const char* str);
void EnableDlgItems2(HWND hwnd, int first, int count, BOOL bEnable);
u32 dlgChk_getBits(HWND hwnd, int first, int count);
void dlgChk_setBits(HWND hwnd, int first, int count, u32 bits);




#define MAX_ARG 16
#define MAX_REG 7
#define SPOIL_MASK_STD 7
#define SPOIL_MASM_WATC 1

enum {
	// x86 register numbers
	REG_EAX, REG_EDX, REG_ECX, REG_EBX
};

enum {
	CALL_TYPE_CDECL,	CALL_TYPE_STDCALL,
	CALL_TYPE_FASTCALL,	CALL_TYPE_THISCALL,
	CALL_TYPE_WATCOM
};

static cch* const regName[] = { "eax", "edx", "ecx", "ebx",
	 "esi", "edi", "ebp" };
	
struct CallSpec {
	cch* type; u32 spoils;
	char args[4];
	
	char get(u32 index) const { return 
		index < 4 ? args[index] : -1; }
	
};
	
static const CallSpec callSpec[] = {
	{ "_cdecl", SPOIL_MASK_STD, {-1,-1,-1,-1}},
	{ "_stdcall", SPOIL_MASK_STD, {-1,-1,-1,-1}},
	{ "_fastcall", SPOIL_MASK_STD, {REG_ECX, REG_EDX,-1,-1}},
	{ "_thiscall", SPOIL_MASK_STD, {REG_ECX,-1,-1,-1}},
	{ "_usercall", SPOIL_MASM_WATC, {REG_EAX, REG_EDX, REG_EBX, REG_ECX}}
};

char* remove_space(char* buff) 
{
	if(!buff) return NULL;
	
	// get start and end
	char *first = 0, *last = buff;
	for(char* str = buff; *str; str++) {
		if(u8(*str) > ' ') { last = str+1;
			if(!first) first = str; }}
			
	// kill whitespace
	*last = 0;
	if(first) { strcpy(buff, first); }
	return *buff ? buff : 0;
}

char* kill_at(char* s)
{
	// remove the @
	s = remove_space(s);
	if(s == NULL) return s;
	char* pos = strrchr(s, '@');
	if(pos) *pos = 0;
	
	// split the string
	pos = strrchr(s, ' ');
	if(!pos) return pos; 	
	if(pos[1] == '*') { pos[0] = '*'; pos++; } 
	*pos = 0;
	return remove_space(pos+1);
}

struct ArgInfo {
	char type[128];	
	char name[128];
	bool chk() { return 
		type[0] || name[0]; }
};

struct CallInfo {
	ArgInfo fn;
	ArgInfo args[MAX_ARG];
	
	char* str(u32 index) {
		ArgInfo* ai = (&fn)+(index/2);
		return index&1 ? ai->name : ai->type; }
		
	void fmt(bstr& str, const CallSpec* spec, u32 spoils);
	u32 spoils(const CallSpec* spec);
};

void fmt_spoils(bstr& str, u32 spoils)
{
	if(spoils == SPOIL_MASK_STD) return;
	
	str.strcat("__spoils<");
	cch* fmt = ", %s"+2;
	for(int i = 0; i < MAX_REG; i++) {
		if(_BTST(spoils, i)) {
			str.fmtcat(fmt, regName[i]); 
			fmt = ", %s"; }}
	str.strcat(">");
}

u32 CallInfo::spoils(const CallSpec* spec)
{
	u32 spoils = spec->spoils;
	for(int i = 0; i < 4; i++) {
		if(!args[i].chk()) break;
		char reg = spec->get(i);
		if(reg >= 0) spoils |= 1<<reg; }
	return spoils;
}

void CallInfo::fmt(bstr& str, 
	const CallSpec* spec, u32 spoils)
{
	// set arg and name
	cch* type = fn.type; if(!*type) type = "void";
	str.fmtcat("%s _%s ", type, spec->type);
	fmt_spoils(str, spoils);
	
	// function name
	cch* name = fn.name; 
	if(!*name) name = "func";	
	str.strcat(name);
	
	// usercall return register
	bool usercall = !strcmp(
		spec->type, "_usercall");
	if(usercall && strcmp(type, "void"))
		str.strcat("@<eax>");
		
	// check varargs
	for(int i = 0; i < MAX_ARG; i++) {
		if(!args[i].chk()) break;
		if(!strcmp(args[i].name, "..."))
			usercall = false; }
	
	str.strcat("(");
	for(int i = 0; i < MAX_ARG; i++) {
		if(!args[i].chk()) break;
		if(i) str.strcat(", ");
		
		// output type
		u32 strPos = str.slen;
		cch* type = args[i].type;
		if(!*type) type = "int";
		str.fmtcat("%s ", type);
		
		// output name
		char* name = args[i].name;
		if(*name) { if(!strcmp(name, "...")) {
				str.slen = strPos; } str.strcat(name); }
		else { str.fmtcat("a%d", i); }
		
		// output reg
		char reg = spec->get(i);
		if(usercall && (reg >= 0)) {
			str.fmtcat("@<%s>", regName[reg]);
		}
	}
	
	str.strcat(");");
}

CallInfo ci;

static cch* const callName[] = { "cdecl", "stdcall", 
	"fastcall", "thiscall", "watcom"};

bool in_edt_update;

void format_call(HWND hwnd)
{
	if(in_edt_update) return;
	in_edt_update = true;
	SCOPE_EXIT(in_edt_update = false);
	
	int iSpec = dlgCombo_getSel(hwnd, IDC_CONVEN);
	auto* spec = callSpec+iSpec;
	
	// get spoils state
	u32 spoils;
	if(IsDlgButtonChecked(hwnd, IDC_SPOIL)) {
		spoils = dlgChk_getBits(hwnd, IDC_SPOIL_EAX, MAX_REG);
	} else {
		spoils = ci.spoils(spec);
		dlgChk_setBits(hwnd, IDC_SPOIL_EAX, MAX_REG, spoils);
	}
	
	Bstr str; ci.fmt(str, spec, spoils);
	SetDlgItemTextA(hwnd, IDC_CALLEDT, str.data);
}

HWND CreateWindowIndirect(LPCREATESTRUCT cs)
{
	return CreateWindowEx(cs->dwExStyle, cs->lpszClass, cs->lpszName, 
		cs->style, cs->x, cs->y, cs->cx, cs->cy, cs->hwndParent,
		cs->hMenu, cs->hInstance, cs->lpCreateParams);
}

void InheritFont(HWND hwnd)
{
	HFONT hFont = (HFONT)sendMessage(GetParent(hwnd), WM_GETFONT);
	sendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, TRUE);
}

HWND CreateChildIndirect(LPCREATESTRUCT cs)
{
	HWND hwnd = CreateWindowIndirect(cs);
	InheritFont(hwnd); return hwnd;
}

CREATESTRUCT GetChildInfo(HWND hwnd)
{
	CREATESTRUCT cs;
	cs.lpCreateParams = 0; cs.lpszName = 0; 
	cs.hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWL_HINSTANCE);
	cs.hMenu = (HMENU)GetWindowLongPtr(hwnd, GWL_ID);
	cs.hwndParent = GetParent(hwnd);
	cs.style = GetWindowLongPtr(hwnd, GWL_STYLE);
	cs.dwExStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
	cs.lpszClass = (LPCTSTR)GetClassLongW(hwnd, GCW_ATOM);
 	XYWH xywh = GetChildXywh(hwnd); cs.x = xywh.x; 
	cs.y = xywh.y; cs.cx = xywh.w; cs.cy = xywh.h;
	return cs;
}

CREATESTRUCT MurderChild2(HWND hwnd) {
	CREATESTRUCT x = GetChildInfo(hwnd);
	DestroyWindow(hwnd); return x; }
CREATESTRUCT GetChildInfo(HWND hwnd, int ctrlId) {
	return GetChildInfo(GetDlgItem(hwnd, ctrlId)); }
CREATESTRUCT MurderChild2(HWND hwnd, int ctrlId) {
	return MurderChild2(GetDlgItem(hwnd, ctrlId)); }
	
void ExpandWindow(HWND hwnd, int x, int y)
{
	RECT rc; GetWindowRect(hwnd, &rc);
	rc.right += x; rc.bottom += y; 
	MoveWindow(hwnd, RECT_XYWH(rc), TRUE);
}

void mainDlgInit(HWND hwnd)
{
	// get child window info
	CREATESTRUCT arg1Text = MurderChild2(hwnd, IDC_ARG1_LABL);
	CREATESTRUCT arg1Type = MurderChild2(hwnd, IDC_ARG1_TYPE);
	CREATESTRUCT arg1Name = MurderChild2(hwnd, IDC_ARG1_NAME);
	int yDelta = MurderChild(hwnd, IDC_ARG2_TYPE).top-arg1Type.y;
	ExpandWindow(hwnd, 0, yDelta*(MAX_ARG-2));
	
	
	// create child windows
	char name[4];
	arg1Text.hMenu = (HMENU)-1;
	arg1Text.lpszName = name;
	for(int i = 0; i < MAX_ARG; i++) {
		sprintf(name, "A%d", i+1);
		
		CreateChildIndirect(&arg1Text); arg1Text.y += yDelta;
		CreateChildIndirect(&arg1Type); arg1Type.y += yDelta;
		CreateChildIndirect(&arg1Name); arg1Name.y += yDelta;
		PTRADD(arg1Type.hMenu, 2); PTRADD(arg1Name.hMenu, 2);
	}

	dlgCombo_addStrs(hwnd, IDC_CONVEN, callName, 5);
	dlgCombo_setSel(hwnd, IDC_CONVEN, 0);
	format_call(hwnd);
}

void edt_update(HWND hwnd, int ctrlId)
{
	// get the text
	char buff[128];
	GetDlgItemTextA(hwnd, ctrlId, buff, 128);
	remove_space(buff);
	
	// update the argument
	int index = ctrlId-IDC_EDIT1;
	printf("%d\n", index);
	
	strcpy(ci.str(index), buff);
	format_call(hwnd);
}

void find_trunc(char* buff, cch* str)
{
	char* x = strstr(buff, str);
	if(x < (buff+2)) return;
	if(x[-1] == '_') x--; 
	if(x[-1] == ' ') x--;
	x[0] = 0;
}

static
int edit_id(int row, int col) {
	return (IDC_EDIT1+col)+row*2; }

void parse_edit(HWND hwnd)
{
	if(in_edt_update) return;
	in_edt_update = true;
	SCOPE_EXIT(in_edt_update = false);
	
	for(int i = IDC_EDIT1; i <= IDC_EDIT1+100; i++) {
		SetDlgItemTextA(hwnd, i, ""); }
	char buff[1024];
	GetDlgItemTextA(hwnd, IDC_CALLEDT, buff, 1024);
	remove_space(buff);

	// get the name
	char* type = strtok(buff, "(");
	if(type){ char* name = kill_at(type);
	if(name){ SetDlgItemTextA(
		hwnd, IDC_EDIT2, name); }}

	// get the type
	if(type){ find_trunc(type, "_spoils");
	for(auto& spec : callSpec) { find_trunc(type, spec.type); }
	if(remove_space(type)){ SetDlgItemTextA
		(hwnd, IDC_EDIT1, type); }}

	// parse arguments
	int row = 1;
	while(char* arg = strtok(NULL, ",);"))
	{
		char* name = kill_at(arg);
		if(!strcmp(arg, "...")) {
			SetDlgItemTextA(hwnd,
				edit_id(row, 1), arg); 
		}	ei(name){	SetDlgItemTextA(hwnd,
			edit_id(row, 0), arg);
		SetDlgItemTextA(hwnd,
			edit_id(row, 1), name); }
		row++;
	}
}

void clipCopy(HWND hwnd)
{
	format_call(hwnd);
	char buff[1024];
	GetDlgItemTextA(hwnd, IDC_CALLEDT, buff, 1024);
	remove_space(buff);
	clipBoad_SetText(buff);
}

void collapse_whitespace(char* str)
{
	unsigned char ch, ch0 = 0;
	char* dst = str;
	while(1) { 
		ch = *str++; 
		if(ch == 0) break;
		if(ch < ' ') { ch = ' ';
			if(ch0 == ' ') continue; }
		*dst++ = ch0 = ch; 
	}
	*dst = ch;
}

void clipPaste(HWND hwnd)
{
	xstr str = clipBoad_GetText();
	collapse_whitespace(str);
	SetDlgItemTextA(hwnd, IDC_CALLEDT, str);
}

void spoilChg(HWND hwnd)
{
	EnableDlgItems2(hwnd, IDC_SPOIL_EAX, MAX_REG,
		IsDlgButtonChecked(hwnd, IDC_SPOIL));
	format_call(hwnd);
}

BOOL CALLBACK mainDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DLGMSG_SWITCH(
		ON_MESSAGE(WM_INITDIALOG, mainDlgInit(hwnd))	
	  CASE_COMMAND(
		  ON_COMMAND(IDC_SPOIL, spoilChg(hwnd))
	    ON_COMMAND(IDC_COPY, clipCopy(hwnd))
	    ON_COMMAND(IDC_PASTE, clipPaste(hwnd))
	    ON_COMMAND(IDCANCEL, EndDialog(hwnd, 0))
			ON_COMMAND(IDC_GENERATE, format_call(hwnd))
			ON_CONTROL(CBN_SELCHANGE, IDC_CONVEN, format_call(hwnd))
			ON_CONTROL(EN_CHANGE, IDC_CALLEDT, parse_edit(hwnd))
			ON_CONTROL_RANGE(EN_CHANGE, IDC_EDIT1, IDC_EDIT1+100,
				edt_update(hwnd, LOWORD(wParam)))
			ON_CONTROL_RANGE(0, IDC_SPOIL_EAX, IDC_SPOIL_EBP, 
				format_call(hwnd));
		,)
	,)
}

int main()
{
	DialogBoxW(NULL, MAKEINTRESOURCEW(IDD_DIALOG1), NULL, mainDlgProc);
}

