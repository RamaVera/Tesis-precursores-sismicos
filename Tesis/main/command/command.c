//
// Created by Ramiro on 6/11/2023.
//

#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include "command.h"
static const char TAG[] = "COMMAND";

esp_err_t COMMAND_Parse(char *rawCommand, command_t *commandParsed){
    if (rawCommand == NULL || strlen(rawCommand) == 0 || commandParsed == NULL) {
        ESP_LOGE(TAG,"Error parsing command rawCommand is null or empty or commandParsed is null");
        return ESP_FAIL;
    }
    char *token = strtok(rawCommand, " ");
    if (strcmp(token, COMMAND_HEADER_TO_RETRIEVE_DATA) == 0 ) {
        commandParsed->header = RETRIEVE_DATA;
        int * fields[] = {
                &commandParsed->startYear,
                &commandParsed->startMonth,
                &commandParsed->startDay,
                &commandParsed->startHour,
                &commandParsed->startMinute,
                &commandParsed->endYear,
                &commandParsed->endMonth,
                &commandParsed->endDay,
                &commandParsed->endHour,
                &commandParsed->endMinute
        };
        const int num_fields = sizeof(fields)/sizeof(fields[0]);
        
        int i = 0;
        int data;
        char * endPtr;
        while (token != NULL && i < num_fields) {
            token = strtok(NULL, "-");
            data = strtol(token, &endPtr, 10);
            if (*endPtr != '\0' || token == endPtr) {
                ESP_LOGE(TAG,"Error parsing command");
                return ESP_FAIL;
            }
            memcpy(fields[i], &data, sizeof(data));
            i++;
        }
        ESP_LOGI(TAG,"Command parsed: %d-%d-%d %d:%d - %d-%d-%d %d:%d",commandParsed->startYear,commandParsed->startMonth,commandParsed->startDay,commandParsed->startHour,commandParsed->startMinute,commandParsed->endYear,commandParsed->endMonth,commandParsed->endDay,commandParsed->endHour,commandParsed->endMinute);
        return ESP_OK;

    }
    else {
        ESP_LOGE(TAG,"Error command header not supported");
        return ESP_FAIL;
    }
}


char * COMMAND_GetHeaderType(command_t aCommand) {
    switch (aCommand.header) {
        case RETRIEVE_DATA:
            return "RETRIEVE DATA COMMAND HEADER";
        default:
            return "COMMAND NOT SUPPORTED";

    }
}

bool COMMAND_matchEndTime(command_t *command, int dayToGet, int hourToGet, int minuteToGet) { return dayToGet == (*command).endDay && hourToGet == (*command).endHour && minuteToGet == (*command).endMinute; }