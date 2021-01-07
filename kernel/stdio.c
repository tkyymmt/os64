#include "stdio.h"
#include "fb.h"
#include "font.h"
#include <stdarg.h>


unsigned int cursor_x = 0, cursor_y = 0;

void putc(char c)
{
	unsigned int x, y;

	switch(c) {
	case '\r':
		cursor_x = 0;
		break;

	case '\n':
		cursor_y += FONT_HEIGHT;
		if ((cursor_y + FONT_HEIGHT) >= fb.vr) {
			cursor_x = cursor_y = 0;
			clear_screen();
		}
		break;

	default:
		/* カーソル座標(cursor_x,cursor_y)へ文字を描画 */
		for (y = 0; y < FONT_HEIGHT; y++)
			for (x = 0; x < FONT_WIDTH; x++)
				if (font_bitmap[(unsigned int)c][y][x])
					draw_px_fg(cursor_x + x, cursor_y + y);

		/* カーソル座標の更新 */
		cursor_x += FONT_WIDTH;
		if ((cursor_x + FONT_WIDTH) >= fb.hr) {
			cursor_x = 0;
			cursor_y += FONT_HEIGHT;
			if ((cursor_y + FONT_HEIGHT) >= fb.vr) {
				cursor_x = cursor_y = 0;
				clear_screen();
			}
		}
	}
}

void puts(char *s)
{
	while (*s != '\0')
		putc(*s++);
}


void putint(int value, const int base)
{
	char buf[32] = {'\0'};
	const int spare = value;
	int i = 0;

	do {
		buf[++i] = "fedcba9876543210123456789abcdef"[15 + value % base];
		value /= base;
	} while (value);

	if (spare < 0)
		buf[++i] = '-';

	for (; i; i--)
		putc(buf[i]);
}

void printf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);

	while (*fmt) {
		if (*fmt == '%') {
			switch (*(++fmt)) {
			case 'c':
			{
				char c = va_arg(args, int);
				putc(c);
				break;
			}
			case 'd':
			{
				int d = va_arg(args, int);
				putint(d, 10);
				break;
			}
			case 'x':
			{
				int x = va_arg(args, int);
				putint(x, 16);
				break;
			}
			case 's':
			{
				const char *s = va_arg(args, char *);
                puts(s);
				break;
			}
			default:
				break;
			}
			++fmt;
			continue;
		}

		putc(*fmt);
		++fmt;
	}

	va_end(args);
}