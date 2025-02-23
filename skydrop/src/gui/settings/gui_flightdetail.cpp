/*
 * gui_flightdetail.cpp
 *
 *  Created on: 22.02.2017
 *      Author: tilmann@bubecks.de
 */

#include <gui/settings/gui_filemanager.h>
#include "gui_flightdetail.h"

#include "../gui_list.h"
#include "../gui_dialog.h"
#include "../../fc/conf.h"
#include "../../fc/logger/logger.h"

uint32_t log_duration;
uint32_t log_odo;
uint32_t log_start;
struct flight_stats_t log_stat;

#define LOG_NODATA_u32	(0xFFFFFFFF)
#define LOG_NODATA_i16	(-32768)

void gui_flightdetail_parse_logfile(const char *filename)
{
	FIL fp;
	FRESULT f_r;
	char line[80];
	char *p;

	// Set defaults, if nothing could be found in the file:
	log_start = LOG_NODATA_u32;
	log_duration = LOG_NODATA_u32;
	log_stat.max_alt = LOG_NODATA_i16;
	log_stat.min_alt = LOG_NODATA_i16;
	log_stat.max_climb = LOG_NODATA_i16;
	log_stat.max_sink = LOG_NODATA_i16;
	log_odo = LOG_NODATA_u32;

	DEBUG("parse_logfile(%s)\n", filename);

	f_r = f_open(&fp, filename, FA_READ);
	if ( f_r != FR_OK ) return;

	// Read from the end of the file
	f_lseek(&fp, f_size(&fp) - 512);

	while(1) {
		if ( f_gets(line, sizeof(line), &fp) == NULL ) break;

		p = strstr_P(line, PSTR("SKYDROP-START-s: "));
		if ( p != NULL ) {
			log_start = atol(p + 17);
			continue;
		}

		p = strstr_P(line, PSTR("SKYDROP-DURATION-ms: "));
		if ( p != NULL ) {
			log_duration = atol(p + 21);
			continue;
		}

		p = strstr_P(line, PSTR("SKYDROP-ALT-MAX-m: "));
		if ( p != NULL ) {
			log_stat.max_alt = atoi(p + 19);
			continue;
		}

		p = strstr_P(line, PSTR("SKYDROP-ALT-MIN-m: "));
		if ( p != NULL ) {
			log_stat.min_alt = atoi(p + 19);
			continue;
		}

		p = strstr_P(line, PSTR("SKYDROP-CLIMB-MAX-cm: "));
		if ( p != NULL ) {
			log_stat.max_climb = atoi(p + 22);
			continue;
		}

		p = strstr_P(line, PSTR("SKYDROP-SINK-MAX-cm: "));
		if ( p != NULL ) {
			log_stat.max_sink = atoi(p + 21);
			continue;
		}

		p = strstr_P(line, PSTR("SKYDROP-ODO-cm: "));
		if ( p != NULL ) {
			log_odo = atol(p + 16);
			continue;
		}
	}

	DEBUG("closing log file\n");
	f_close(&fp);
}

void gui_flightdetail_init()
{
	char tmp[44];

	sprintf_P(tmp, PSTR("%s/%s"), gui_filemanager_path, gui_filemanager_name);
	gui_flightdetail_parse_logfile(tmp);

	gui_list_set(gui_flightdetail_item, gui_flightdetail_action, 8, GUI_FILEMANAGER);
}

void gui_flightdetail_stop() {}

void gui_flightdetail_loop()
{
	gui_list_draw();
}

void gui_flightdetail_irqh(uint8_t type, uint8_t * buff)
{
	gui_list_irqh(type, buff);
}

void gui_flightdetail_action(uint8_t index) {}

void gui_flightdetail_item(uint8_t idx, char * text, uint8_t * flags, char * sub_text)
{
	uint32_t diff;
	uint8_t sec, min, hour, day, wday, month;
	uint16_t year;

	*flags |= GUI_LIST_SUB_TEXT;

	switch (idx)
	{
		case 0:
			strcpy_P(text, PSTR("Start (date):"));
			if (log_start == LOG_NODATA_u32)
			{
				sprintf_P(sub_text, PSTR("<no data>"));
				break;
			}

			datetime_from_epoch(log_start, &sec, &min, &hour, &day, &wday, &month, &year);

			sprintf_P(sub_text, PSTR("%02d.%02d.%04d"), day, month, year);
		break;

		case 1:
			strcpy_P(text, PSTR("Start (time):"));
			if (log_start == LOG_NODATA_u32)
			{
				sprintf_P(sub_text, PSTR("<no data>"));
				break;
			}

			datetime_from_epoch(log_start, &sec, &min, &hour, &day, &wday, &month, &year);

			sprintf_P(sub_text, PSTR("%02d:%02d.%02d"), hour, min, sec);
		break;

		case 2:
			strcpy_P(text, PSTR("Duration:"));
			if (log_duration == LOG_NODATA_u32)
			{
				sprintf_P(sub_text, PSTR("<no data>"));
				break;
			}

			diff = log_duration / 1000;
			hour = diff / 3600;
			diff %= 3600;
			min = diff / 60;
			diff %= 60;

			if (hour > 0)
				sprintf_P(sub_text, PSTR("%02d:%02d"), hour, min);
			else
				sprintf_P(sub_text, PSTR("%02d.%02d"), min, diff);

		break;
		case 3:
			strcpy_P(text, PSTR("Altitude(max):"));
			if (log_stat.max_alt == LOG_NODATA_i16)
			{
				sprintf_P(sub_text, PSTR("<no data>"));
				break;
			}

			sprintf_P(sub_text, PSTR("%dm"), log_stat.max_alt);
		break;
		case 4:
			strcpy_P(text, PSTR("Altitude(min):"));
			if (log_stat.min_alt == LOG_NODATA_i16)
			{
				sprintf_P(sub_text, PSTR("<no data>"));
				break;
			}

			sprintf_P(sub_text, PSTR("%dm"), log_stat.min_alt);
		break;
		case 5:
			strcpy_P(text, PSTR("Sink(max):"));
			if (log_stat.max_sink == LOG_NODATA_i16)
			{
				sprintf_P(sub_text, PSTR("<no data>"));
				break;
			}

			sprintf_P(sub_text, PSTR("%0.1fm/s"), log_stat.max_sink / 100.0);
		break;
		case 6:
			strcpy_P(text, PSTR("Climb(max):"));
			if (log_stat.max_climb == LOG_NODATA_i16)
			{
				sprintf_P(sub_text, PSTR("<no data>"));
				break;
			}

			sprintf_P(sub_text, PSTR("%0.1fm/s"), log_stat.max_climb / 100.0 );
		break;
		case 7:
			strcpy_P(text, PSTR("Odometer:"));
			if (log_odo == LOG_NODATA_u32)
			{
				sprintf_P(sub_text, PSTR("<no data>"));
				break;
			}

			sprintf_P(sub_text, PSTR("%0.1fkm"), log_odo / (100 * 1000.0));
		break;
//		case 7:
//			strcpy_P(text, PSTR("Delete"));
//			*flags |= 0;
//		break;
	}

}

