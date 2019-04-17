/*
 * ILI9341_TFT_LCD_ATMEGA32.c
 *
 * Created: 2018-11-06 22:21:06
 * Author : Daniel
 * Wersja 2.0
 * Obsluga dotyku, obsluga kart SD (petitFS), grafika, nowa czcionka Arial16x16
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include "ILI9341.h"
#include "colors.h"
#include "touch.h"
#include "font16x16.h"
#include "font5x7.h"
#include "font8x8.h"
#include "font8x12.h"
#include "Arial16x16.h"
#include "literals.h"
//#include "bmp_color.h"
#include "bmp_mono.h"
// obsluga karty SD - biblioteki petitFS
#include "diskio.h"
#include "pff.h"
#include "sd_card.h"

#define PGM_GETSTR(str, idx) (char *)pgm_read_word(&str[idx])

void touch_calibration(void);
void load_picture(const char *file_name, uint16_t x, uint16_t y);
void set_layout(void);

uint16_t drawColor = GREEN;

int main(void)
{
    ILI9341_init();							// inicjalizacja wyswietlacza
	XPT2046_init_io();						// inicjalizacja dotyku
	XPT2046_rd_ee_cal();					// martryca kalibracji
	SDCARD_init_io();						// Inicjacja karty SD
	ILI9341_set_rotation(LANDSCAPE);		// ustawienie obrazu w poziomie
	//touch_calibration();					// odpalane jednorazowo aby skalibrowac dotyk i obraz.

	set_layout();

	//rysowanie grafiki
	//load_picture("GRAFIKA.BIN", 0, 0);
	//ILI9341_draw_color_bmp(120, 100, 32, 32, suzuki);
	ILI9341_draw_mono_bmp(120, 100, 32, 32, znak, WHITE, GRAYBLUE);
	ILI9341_draw_mono_bmp(160, 140, 32, 32, cross, WHITE, GRAYBLUE);
	
	ILI9341_set_font((font_t) {Arial16x16, 16, 16, CYAN, TRANSPARENT});
	ILI9341_txt(60, 60, "DANIEL");	
	
    while (1) 
    {
		XPT2046_rd_touch();					// odczyt wspolrzednych na digitizerze
		
		if (touch.ok)
		{
			ILI9341_draw_pixel(touch.x_cal, touch.y_cal, drawColor);
			ILI9341_draw_pixel(touch.x_cal - 1, touch.y_cal, drawColor);
			ILI9341_draw_pixel(touch.x_cal, touch.y_cal - 1, drawColor);
			ILI9341_draw_pixel(touch.x_cal, touch.y_cal + 1, drawColor);
			
			// instrukcja IF do zrobienia aktywnych pol pod guziki.
			if(touch.y_cal < 30)
			{
				if (touch.x_cal > 10 && touch.x_cal < 50)
					drawColor = RED;
				if (touch.x_cal > 60 && touch.x_cal < 100)
					drawColor = GREEN;
				if (touch.x_cal > 110 && touch.x_cal < 150)
					drawColor = BLUE;
				if (touch.x_cal > 160 && touch.x_cal < 200)
					drawColor = YELLOW;
				if (touch.x_cal > 210 && touch.x_cal < 250)
					drawColor = CYAN;
				if (touch.x_cal > 260 && touch.x_cal < 300)
					{
						ILI9341_cls(BLACK);
						load_picture("ZOLW.BIN", 0, 0);		
					}
			}
		}
    }
}

void set_layout(void)
{
	ILI9341_draw_rectangle(5, 30, 315, 235, YELLOW);			// prostokat
	ILI9341_draw_fast_rect(10, 5, 40, 20, true, RED);			// czerwony
	ILI9341_draw_fast_rect(60, 5, 40, 20, true, GREEN);		// zielony
	ILI9341_draw_fast_rect(110, 5, 40, 20, true, BLUE);		// niebieski
	ILI9341_draw_fast_rect(160, 5, 40, 20, true, YELLOW);		// zolty
	ILI9341_draw_fast_rect(210, 5, 40, 20, true, CYAN);		// cyan
	ILI9341_draw_fast_rect(260, 5, 40, 20, true, LGRAY);	// RESET
	ILI9341_draw_line(260, 5, 300, 25, RED);
	ILI9341_draw_line(300, 5, 260, 25, RED);				// iksik	
}

void touch_calibration(void)
{
	ILI9341_cls(BLACK);
	ILI9341_set_font((font_t) {font16x16, 16, 16, YELLOW, BLACK});
	int32_t x, y;
	point_t sample_points[3];
	
	for (uint8_t cal_step = 0; cal_step < 3; cal_step++)		// beda 3 punkty kalibracyjne
	{
		ILI9341_txt_P(0, 120, PGM_GETSTR(calibration_txt, cal_prompt_idx));
		eeprom_read_block(&x, &ee_cal_points[cal_step].x, sizeof(x));
		eeprom_read_block(&y, &ee_cal_points[cal_step].y, sizeof(y));
		ILI9341_draw_circle(x, y, 5, true, WHITE);			// rysuje kolko kalibracyjne w tych wspolrzednych
		
		do 
		{
			do 
			{
				XPT2046_rd_touch();
			} 
			while (!touch.ok);
			
			sample_points[cal_step].x = touch.x_raw;		// pobiera wspolrzedna X
			sample_points[cal_step].y = touch.y_raw;		// pobiera wspolrzedna Y
			
			if(labs(x - touch.x_raw) > MAX_CAL_ERROR || labs(y - touch.x_raw) > MAX_CAL_ERROR)
			{
				ILI9341_set_font((font_t) {font16x16, 16, 16, RED, BLUE});
				ILI9341_txt_P(0, 120, PGM_GETSTR(calibration_txt, cal_bad_idx));
			}
			else
			{
				ILI9341_set_font((font_t) {font16x16, 16, 16, GREEN, BLUE});
				ILI9341_txt_P(0, 120, PGM_GETSTR(calibration_txt, cal_ok_idx));
			}
		} 
		while (labs(x - touch.x_raw) > MAX_CAL_ERROR || labs(y - touch.y_raw) > MAX_CAL_ERROR);
		_delay_ms(500);
		ILI9341_draw_circle(x, y, 5, true, BLACK);		// usuwa stare kolko kalibracyjne: zamalowuje je na czarno
	}
	set_cal_matrix(sample_points);						// tworzy macierz kalibracyjna
	XPT2046_wr_ee_cal();
	ILI9341_set_font((font_t) {font16x16, 16, 16, YELLOW, BLUE});
	ILI9341_txt_P(0, 120, PGM_GETSTR(calibration_txt, cal_end_idx));
}

void load_picture( const char *file_name, uint16_t x, uint16_t y)																
{
	char buffer[512];
	UINT bytes_read;
	FATFS fs;
	uint8_t sector;

	if (disk_initialize() == FR_OK)
	{
		if (pf_mount(&fs) == FR_OK)
		{
			if (pf_open(file_name) == FR_OK)
			{
				sector = 0;													// Pierwszy sektor
				uint8_t start_pos;											// Pozycja startowa

				do
				{
					bytes_read = 0;											// odczytane bajty
					pf_read(buffer, sizeof(buffer), &bytes_read);			// czytaj czesc pliku do bufora (dlugosc 512 bity)

					if (!(sector))											// jesli pierwszy sektor to zero
					{
						ILI9341_set_window(x, y, buffer[0] + (buffer[1] << 8) - 1, buffer[2] + (buffer[3] << 8) - 1);	// Ustawienie wymiarow grafiki
						start_pos = 4;										// dane od pozycji 4
					}
					else start_pos = 0;										// dane od pozycji 0

					// sprawdzenie czy sektor zapelniony czy nie.
					for (uint16_t pixel = start_pos; pixel < (bytes_read % sizeof(buffer) ? bytes_read % sizeof(buffer) : sizeof(buffer)); pixel += 2)
					{
						ILI9341_push_color(buffer[pixel] + (buffer[pixel + 1] << 8));
					}

					sector++;
				}
				while (!(bytes_read % sizeof(buffer)));						//Powtarzaj az caly plik zostanie wczytany
			}
			else ILI9341_txt(0, 0, "Blad pliku");				// Brak pliku
		}
		else ILI9341_txt(0, 0, "Blad zamontowania");				// Brak systemu FAT
	}
	else ILI9341_txt(0, 0, "Blad karty");					// Brak karty
}
	


