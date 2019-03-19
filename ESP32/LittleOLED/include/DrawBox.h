/*
 * DrawBox.h
 *
 *  Created on: 5 Dec 2018
 *      Author: xasin
 */

#ifndef XASCODE_PATH_LITTLEOLED_DRAWBOX_H_
#define XASCODE_PATH_LITTLEOLED_DRAWBOX_H_

#include <vector>
#include <functional>

#include "fonttype.h"

#ifndef DEFAULT_FONT
#define DEFAULT_FONT console_font_6x8
#endif

#define MAX_BRIGHTNESS 2

namespace Peripheral {
namespace OLED {

struct DirtyArea {
	uint8_t startX;
	uint8_t endX;
	uint8_t startY;
	uint8_t endY;
};

class DrawBox {
private:
	int width;
	int height;

	DrawBox *topBox;

protected:
	std::vector<DrawBox *>bottomBoxes;

	virtual void redraw();

public:
	int  offsetX;
	int  offsetY;

	char rotation;

	bool transparent;

	bool visible;
	bool inverted;

	std::function<void (void)> onRedraw;

	DrawBox(int width, int height);
	DrawBox(int width, int height, DrawBox *headBox);
	virtual ~DrawBox();

	void set_head(DrawBox *headBox, bool registerCB = true);

	virtual void request_redraw();

	virtual void set_pixel(int x, int y, int8_t brightness = 3);

	virtual void draw_line(int x, int y, int l, int r, int8_t brightness = 3);
	virtual void draw_box(int x, int y, int width, int height, int8_t oBrightness = 3, int8_t iBrightness = -1);

	int get_line_width(FontType *font = nullptr);
	int get_lines(FontType *font = nullptr);

	void write_char(int x, int y, char c, int8_t foreground = 3, int8_t background = -1, FontType *font = nullptr);
	void write_string(int x, int y, const std::string oString, int8_t foreground = 3, int8_t background = -1, FontType *font = nullptr);
};

} /* namespace OLED */
} /* namespace Peripheral */

#endif /* XASCODE_PATH_LITTLEOLED_DRAWBOX_H_ */
