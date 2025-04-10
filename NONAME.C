#include <mem.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include <sys/types.h>

typedef unsigned char BYTE;
typedef unsigned int WORD;
typedef unsigned long DWORD;

extern BYTE SetTandyMediumMode();
extern BYTE IsInGraphicsMode();
extern void SetTextMode();
extern void CopyPage(BYTE far *source, BYTE far *dest);
extern void Cls(BYTE far *buffer, BYTE color);
extern void Pset(BYTE far *buffer, int x, int y, unsigned char color);

int allocatedPages = 0;

int SetActivePage(unsigned char active, unsigned char visible)
{
	unsigned char activePage = 0xC0;

	if ((active >= allocatedPages)
			|| (visible >= allocatedPages))
	{
		return -1;
	}

	activePage |= (active << 4) | (visible << 1);

	asm {
		MOV DX,0x3DF;
		MOV AL, activePage;
		OUT DX,AL;
	}

	return 0;
}

int AllocatePages(int count)
{
	int delta = 0;
	int kBytes = 0;
	int available = 0;
	int total = 0;
	int paraCount = 0;

	if (count < 0)
	{
		// Invalid parameter.
		return -1;
	}

	if (count == allocatedPages)
	{
		return count;
	}

	delta = allocatedPages - count;

	if (delta == 0)
	{
		return count;
	}

	// Amount that we're allocating
	// (negative means taking memory, positive means returning).
	kBytes = (delta > 0 ? 1 : -1) * ((abs(delta) * 32) + 16);

	if (kBytes < -112)
	{
		// Not enough memory.
		return -2;
	}

	asm {
		PUSH DS

		MOV AX, 0x40
		MOV DS, AX
		MOV AX,[DS:0x15];
		MOV BX,[DS:0x13];

		MOV [total], AX
		MOV [available], BX

		POP DS
	}

	// 128k expansion found, we're good to go.
	if (total == available)
	{
		allocatedPages = count;
		return count;
	}

	if (abs(kBytes) > available)
	{
		// Not enough memory.
		return -2;
	}

	// Calculate paragraphs.
	paraCount = kBytes << 6;

	asm {
		PUSH DS

		MOV BX, DS
		DEC BX
		MOV DS, BX
		MOV BX, [DS:3]
		ADD BX, [paraCount]
		MOV [DS:3], BX
		MOV BX, 0x40
		MOV DS, BX
		MOV BX, [DS:0x13]
		ADD BX, [kBytes]
		MOV [DS:0x13], BX
		POP DS

		MOV AH,0x49
		MOV DX,0x2C
		MOV ES,DX
		INT 21
	}

	allocatedPages = count;

	return count;
}

void main()
{
	BYTE far *screen = (BYTE far *)0xB8000000;
	BYTE far *buffer = (BYTE far *)malloc(32768);
	int allocResult = 0;

	int x, y, color, hflip, vflip, vpage, apage, temp;
	int xacc = 1, yacc = 1;
	time_t timet;

	srand((unsigned)time(&timet));

	if (buffer == NULL)
	{
		printf("FUCK.\n");
		return;
	}

	allocResult = AllocatePages(2);

	if (allocResult == -2)
	{
		printf("Not enough memory.");
	}

	if (SetTandyMediumMode() != 1)
	{
		SetTextMode();
		printf("Not a Tandy 1000.\n");
		return;
	}

	Cls(buffer, 0);
	Cls(screen, 4);

	Pset(screen, 0, 99, 15);
	Pset(screen, 319, 99, 15);

	for (y = 0; y < 200; y += 2)
	{
		for (x = 0; x < 320; x += 2)
		{
			Pset(buffer, x, y, 12);
			Pset(buffer, x + 1, y, 10);
			Pset(buffer, x, y + 1, 10);
			Pset(buffer, x + 1, y + 1, 12);
		}
	}

	vpage = 0;
	apage = 1;
	x = 319;
	y = 100;
	hflip = 0;
	vflip = 0;
	temp = 0;

	while(!kbhit())
	{
		if (SetActivePage(apage, vpage) != 0)
		{
			SetTextMode();
			printf("Not a valid page.");
			return;
		}

		CopyPage(buffer, screen);

		Pset(screen, x - 1, y, 0);
		Pset(screen, x - 2, y, 9);

		Pset(screen, x, y + 1, 1);
		Pset(screen, x - 1, y + 1, 1);
		Pset(screen, x - 2, y + 1, 9);
		Pset(screen, x - 3, y + 1, 11);

		Pset(screen, x, y + 2, 0);
		Pset(screen, x - 1, y + 2, 1);
		Pset(screen, x - 2, y + 2, 9);
		Pset(screen, x - 3, y + 2, 11);

		Pset(screen, x - 1 , y + 3, 0);
		Pset(screen, x - 2 , y + 3, 9);

		temp = apage;
		apage = vpage;
		vpage = temp;

		SetActivePage(apage, vpage);

		if (!hflip)
		{
			x -= xacc;
		}
		else
		{
			x += xacc;
		}

		if (!vflip)
		{
			y += yacc;
		}
		else
		{
			y -= yacc;
		}

		if (x < 0)
		{
			x = 0;
			hflip = 1;
			xacc = random(5) + 1;
		}
		else if (x > 323)
		{
			x = 323;
			hflip = 0;
			xacc = random(5) + 1;
		}

		if (y < -4)
		{
			y = -4;
			vflip = 0;
			yacc = random(5) + 1;
		}
		else if (y > 200)
		{
			y = 200;
			vflip = 1;
			yacc = random(5) + 1;
		}
	}

	SetActivePage(0, 0);

	getch();

	AllocatePages(0);
	SetTextMode();
}