/*
 * SSD1306.cpp
 *
 *  Created on: 29 Nov 2018
 *      Author: xasin
 */

#include "SSD1306.h"

namespace Peripheral {
namespace OLED {

#include "fonttype.h"

void SSD1306::call_raw_update(void *args) {
	uint32_t dummy;
	while(true) {
		xTaskNotifyWait(0, 0, &dummy, 1000/portTICK_PERIOD_MS);

		reinterpret_cast<SSD1306 *>(args)->raw_update();
	}
}

SSD1306::SSD1306() :
		DrawBox(128, 64),

		currentAction(nullptr), cmdBuffer(),
		screenBuffer(),
		updateTask(nullptr) {
}

void SSD1306::initialize() {
	vTaskDelay(100);

	start_cmd_set();

	send_cmd(DISPLAY_OFF);
	
	/// HARDWARE CONFIGS
	send_cmd(SET_SEG_REMAP);
	send_cmd(SET_CHARGE_PUMP, 0x14);
	send_cmd(SET_MUX_RATIO, 0x1F);
	send_cmd(SET_CLK_DIV, 0x80);
	send_cmd(SET_PRECHARGE, 0x1F);
	send_cmd(SET_COM_PIN_MAP, 0x12);
	send_cmd(0xDB, 0x40);

	end_cmd_set();

	vTaskDelay(100/portTICK_PERIOD_MS);

	start_cmd_set();
	send_cmd(SET_MEMORY_ADDR_MODE, 2);

	send_cmd(0xC0);
	send_cmd(0x40);

	// DISPLAY CONFIGS
	send_cmd(SET_DISPLAY_OFFSET, 0);
	send_cmd(SET_CONTRAST, 0x4F);

	send_cmd(DISPLAY_NONINV);
	send_cmd(DISPLAY_ON);
	send_cmd(DISPLAY_RAM);

	end_cmd_set();

	vTaskDelay(100/portTICK_PERIOD_MS);

	push_entire_screen();

	xTaskCreate(SSD1306::call_raw_update, "SSD1306 Updater", 4096, this, 3, &updateTask);
	puts("SSD initialized!");
}

XNM::I2C::MasterAction* SSD1306::start_cmd_set() {
	assert(currentAction == nullptr);
	this->currentAction = new XNM::I2C::MasterAction(0b0111100);

	return currentAction;
}

void SSD1306::send_cmd(uint8_t cmdByte) {
	assert(currentAction != nullptr);

	cmdBuffer.push_back(cmdByte);
}
void SSD1306::send_cmd(uint8_t cmdByte, uint8_t param) {
	assert(currentAction != nullptr);

	cmdBuffer.push_back(cmdByte);
	cmdBuffer.push_back(param);
}

void SSD1306::end_cmd_set() {
	assert(currentAction != nullptr);

	currentAction->write(0x00, cmdBuffer.data(), cmdBuffer.size());

	currentAction->execute();

	cmdBuffer.clear();
	delete currentAction;
	currentAction = nullptr;
}

void SSD1306::data_write(void *data, size_t length) {
	assert(currentAction == nullptr);

	currentAction = new XNM::I2C::MasterAction(0b0111100);

	currentAction->write(DATA_STREAM, data, length);
	currentAction->execute();

	delete currentAction;
	currentAction = nullptr;
}

void SSD1306::set_page(uint8_t page) {
	this->currentAction = start_cmd_set();

	const uint8_t oData[] = {
		uint8_t(0xB0 + page),
		4, 0x12
	};
	for(uint8_t i=0; i<sizeof(oData); i++)
		cmdBuffer.push_back(oData[i]);

	end_cmd_set();
}

void SSD1306::clear() {
	for(auto &page : screenBuffer) {
		for(uint8_t i=0; i<page.size(); i++)
			page[i] = 0;
	}
}
void SSD1306::push_entire_screen() {
	for(uint8_t page = 0; page < screenBuffer.size(); page++) {
		set_page(page);
		data_write(screenBuffer[page].begin(), screenBuffer[page].size());
	}
}

void SSD1306::request_redraw() {
	xTaskNotifyFromISR(updateTask, 0, eNoAction, nullptr);
}

void SSD1306::raw_update() {
	this->clear();
	this->redraw();

	this->push_entire_screen();
}

void SSD1306::set_pixel(int x, int y, int8_t brightness) {
	if(brightness < 0)
		return;
	if(x < 0)
		return;
	if(y < 0)
		return;

	uint8_t page   = y / 8;
	uint8_t page_y = y%8;

	if(page >= screenBuffer.size())
		return;
	if(x >= screenBuffer[page].size())
		return;

	uint8_t &dByte = screenBuffer[page][x];

	if(brightness > 1)
		dByte |= 1<<page_y;
	else
		dByte &= ~(1<<page_y);
}

} /* namespace OLED */
} /* namespace Peripheral */
