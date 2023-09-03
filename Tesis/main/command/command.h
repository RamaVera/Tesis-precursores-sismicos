//
// Created by Ramiro on 6/11/2023.
//

#ifndef COMMAND_H
#define COMMAND_H

#include <esp_err.h>
#include <esp_log.h>
#include <string.h>

typedef enum commandHeader{
    RETRIEVE_DATA
}commandHeader_t;

typedef struct command {
    commandHeader_t header;
    int startYear;
    int startMonth;
    int startDay;
    int startHour;
    int startMinute;
    int endYear;
    int endMonth;
    int endDay;
    int endHour;
    int endMinute;
} command_t;

static char *const COMMAND_HEADER_TO_RETRIEVE_DATA = "R";

esp_err_t COMMAND_Parse(char *rawCommand,command_t *commandParsed);
char *COMMAND_GetHeaderType(command_t aCommand);
bool COMMAND_matchEndTime(command_t *command, int dayToGet, int hourToGet, int minuteToGet);

#endif //COMMAND_H
