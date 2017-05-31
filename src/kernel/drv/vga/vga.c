/*

PSOS Development Build
https://github.com/TheBenPerson/PSOS/tree/dev

Copyright (C) 2016 - 2017 Ben Stockett <thebenstockett@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial vPortions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "kernel.h"
#include "math.h"
#include "types.h"
#include "vga.h"

word *modes = NULL;
byte vAttr = BG_GREEN;
word vPort;
word vOffset = 0;

struct __attribute__((packed)) {

	char signature[4];
	word version;
	dword oemString; //far pointer
	byte caps[4];
	dword modes; //far pointer
	word mem;
	byte reserved[236];

} vbeInfo;

void scroll();
void setBank(word bank);

void initVGA() {

	word tvPort;

	asm("mov es, %0" :: "a" (0));
	asm("mov %0, es:[0x463]" : "=g" (tvPort)); //address 0x0:0x463 in BDA is base VGA vPort
	vPort  = tvPort;

	word result;

	asm("mov ax, ds");
	asm("mov es, ax");
	asm("int 0x10"
		: "=a" (result)
		: "a" (0x4F00), "D" (&vbeInfo));

	if (result == 0x4F) modes = vbeInfo.modes & 0xFFFF;

	kclearText();
	ksetCursor(false);

}

void kclearText() {

	asm("mov es, %0" :: "r" (0xB800));

	for (word i = 0; i < (80 * 25 * 2); i += 2) {

		asm("movw es:[%0], %1" :: "b" (i), "g" (vAttr << 8));

	}

	vOffset = 0;

}

void kputc(char c) {

	asm("mov es, %0" :: "a" (0xB800));

	if (vOffset >= (80 * 25 * 2)) {

		scroll();
		vOffset = (80 * 24 * 2);

	}

	if (c == '\n') {

		vOffset += 160 - (vOffset % 160);

	} else {

		asm("mov es:[%0], %1" :: "b" (vOffset), "r" (c));
		asm("mov es:[bx + 1], %0" :: "r" (vAttr));

		vOffset += 2;

	}

	ksetPosition(vOffset);

}

void kputn(word num, bool hex) {

	byte base = hex ? 16 : 10;
	word tNum = num;

	byte place = 1;
	while (tNum >= base) {

		tNum /= base;
		place++;

	}

	char string[place + 1];
	string[place] = '\0';

	for (byte i = 0; i < place; i++) {

		byte value = (num % base) + 0x30;
		if (value > 0x39) value += 0x7;

		string[place - i - 1] = value;
		num /= base;

	}

	bool tSyscalled = syscalled;
	syscalled = false;
	if (hex) kputs("0x");
	kputs(string);
	syscalled = tSyscalled;

}

void kputs(mem16_t string) {

	word tSegment = syscalled ? uSegment : KERNEL_SEGMENT;

	asm("mov es, %0" :: "r" (0xB800));

	for (;;) {

		REMOTE();
		char c = *((char*) string);
		LOCAL();

		if (!c) {

			ksetPosition(vOffset);
			return;

		}

		if (vOffset >= (80 * 25 * 2)) {

			scroll();
			vOffset = (80 * 24 * 2);

		}

		if (c == '\n') {

			vOffset += 160 - (vOffset % 160);

		} else {

			asm("mov es:[%0], %1" :: "b" (vOffset), "r" (c));
			asm("mov es:[bx + 1], %0" :: "r" (vAttr));

			vOffset += 2;

		}

		string++;

	}

}

void ksetCursor(bool enabled) {

	asm("int 0x10" :: "a" (1 << 8), "c" ((word) (enabled ? 0x7 : 0x700)));

}

word ksetPosition(word position) {

	if (position == 0xFFFF) return vOffset;

	vOffset = position;
	position /= 2;

	outb(vPort, 0xF);
	outb(vPort + 1, position & 0xFF);
	outb(vPort, 0xE);
	outb(vPort + 1, position >> 8);

	return 0;

}

bool ksetVGAMode(word width, word height, bool graphical) {

	if (!modes) return false;

	word mode;
	byte i = 0;

	asm("mov ax, ds");
	asm("mov es, ax");

	for (;;) {

		mode = modes[i++];
		if (mode == 0xFFFF) return false;

		struct __attribute__((packed)) {

			word attributes;
			byte winA;
			byte winB;
			word granularity;
			word winsize;
			word segmentA;
			word segmentB;
			dword realFctPtr; //far pointer
			word pitch;

			word width;
			word height;
			byte charWidth;
			byte charHeight;
			byte planes;
			byte bpp;
			byte banks;
			byte memory_model;
			byte bank_size;
			byte image_pages;
			byte reserved0;

			byte red_mask;
			byte red_position;
			byte green_mask;
			byte green_position;
			byte blue_mask;
			byte blue_position;
			byte rsv_mask;
			byte rsv_position;
			byte directcolor_attributes;

			dword physbase;
			byte reserved[212];

		} modeInfo;

		word result;

		asm("int 0x10"
			: "=a" (result)
			: "a" (0x4F01), "c" (mode), "D" (&modeInfo));

		if (result != 0x4F) continue;

		//if (graphical && (modeInfo.attributes & 0x80)) continue; //no linear frame buffer
		if (!(modeInfo.attributes & 0x10) == graphical) continue;
		if (modeInfo.bpp != 24) continue;

		if (modeInfo.width != width) continue;
		if (modeInfo.height != height) continue;

		break;

	}

	word result;

	asm("int 0x10"
		: "=a" (result)
		: "a" (0x4F02), "b" (mode));

	asm("mov es, %0" :: "a" (0xA000));

	for (byte i = 0; i < 15; i++) {

		setBank(i);

		for (word j = 0; j < 0xFFFF; j += 3) {

			asm("mov es:[%0], %1" :: "b" (j), "a" ((byte) j & 0xFF));
			asm("mov es:[%0], %1" :: "b" (j + 1), "a" ((byte) 0));
			asm("mov es:[%0], %1" :: "b" (j + 2), "a" ((byte) 0));

		}

	}

	return (result == 0x4F);

}

void scroll() {

	asm("mov es, %0" :: "r" (0xB800));

	for (size_t i = 0; i < (80 * 24 * 2); i += 2) {

		asm("movw ax, es:[%0 + 160]" :: "b" (i));
		asm("movw es:[bx], ax");

	}

	for (size_t i = (80 * 24 * 2); i < (80 * 25 * 2); i += 2)
		asm("mov es:[%0], %1" :: "b" (i), "a" (vAttr << 8));

}

void setAttr(byte attr) {

	vAttr = attr;

}

void setBank(word bank) {

	asm("int 0x10" :: "a" (0x4F05), "b" ((word) 0), "d" (bank));

}
