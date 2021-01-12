#include "stdio.h"
#include "fb.h"
#include "font.h"
#include "kbc.h"
#include <stdarg.h>


unsigned int cursor_x = 0, cursor_y = 0;


void putchar(const char c)
{
	switch(c) {
	case '\r':
		cursor_x = 0;
		break;

	case '\n':
		cursor_x = 0;
		cursor_y += FONT_HEIGHT;
		if ((cursor_y + FONT_HEIGHT) >= fb.vr) {
			cursor_y = 0;
			clear_screen();
		}
		break;

	default:
		/* カーソル座標(cursor_x,cursor_y)へ文字を描画 */
		for (int y = 0; y < FONT_HEIGHT; y++) {
			if (!font_8x14[c][y])
				continue;
			for (int x = 0; x < FONT_WIDTH; x++)
				if (((font_8x14[c][y] >> x) & 1))
					draw_px_fg(cursor_x + x, cursor_y + y);
		}
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
		putchar(*s++);
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
		putchar(buf[i]);
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
				putchar(c);
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

		putchar(*fmt);
		++fmt;
	}

	va_end(args);
}


const char getchar()
{
	return normal_keymap[get_keycode()];
}