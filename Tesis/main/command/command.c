//
// Created by Ramiro on 6/11/2023.
//

#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include "command.h"
static const char TAG[] = "COMMAND";

//#define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT_COMMAND(tag, fmt, ...) ESP_LOGI(tag, fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT_COMMAND(tag, fmt, ...) do {} while (0)
#endif

esp_err_t COMMAND_Parse(char *rawCommand, command_t *commandParsed){
    if (rawCommand == NULL || strlen(rawCommand) == 0 || commandParsed == NULL) {
        ESP_LOGE(TAG,"Error parsing command rawCommand is null or empty or commandParsed is null");
        return ESP_FAIL;
    }
    char *token = strtok(rawCommand, " ");
    if (strcmp(token, COMMAND_HEADER_TO_RETRIEVE_DATA) == 0 ) {
        commandParsed->header = RETRIEVE_DATA;
	    if ( parseDateCommand( commandParsed, token ) != ESP_OK ) {
		    return ESP_FAIL;
	    }
		return ESP_OK;

    } else if (strcmp(token, COMMAND_HEADER_TO_DELETE_FILES) == 0 ) {
		commandParsed->header = DELETE_FILES;
	    if ( parseDateCommand( commandParsed, token ) != ESP_OK ) {
		    return ESP_FAIL;
	    }
	    return  ESP_OK;
	
    } else if (strcmp(token, COMMAND_HEADER_TO_QUERY_SPACE) == 0 ) {
	    commandParsed->header = QUERY_SD_SPACE;
	    return ESP_OK;
    } else if (strcmp(token, COMMAND_HEADER_TO_REBOOT) == 0 ) {
	    commandParsed->header = REBOOT;
	    return ESP_OK;
    } else {
        ESP_LOGE(TAG,"Error command header not supported");
        return ESP_FAIL;
    }
}

esp_err_t parseDateCommand ( command_t *commandParsed, char *token ) {
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
	DEBUG_PRINT_COMMAND( TAG, "Command parsed: %d-%02d-%02d %02d:%02d - %d-%02d-%02d %02d:%02d",
	                     commandParsed->startYear, commandParsed->startMonth, commandParsed->startDay,
	                     commandParsed->startHour, commandParsed->startMinute, commandParsed->endYear,
	                     commandParsed->endMonth, commandParsed->endDay, commandParsed->endHour,
	                     commandParsed->endMinute );
	return ESP_OK;
}


char * COMMAND_GetHeaderType(command_t aCommand) {
    switch (aCommand.header) {
        case RETRIEVE_DATA:
            return "RETRIEVE DATA COMMAND HEADER";
        default:
            return "COMMAND NOT SUPPORTED";

    }
}

bool COMMAND_matchEndTime(command_t *command, int dayToGet, int hourToGet, int minuteToGet) {
	// Convert the times to minutes since the start of the month for comparison
	int commandEndTime = ((*command).endDay * 24 * 60) + ((*command).endHour * 60) + (*command).endMinute;
	int currentTime = (dayToGet * 24 * 60) + (hourToGet * 60) + minuteToGet;
	
	// Return true only if the current time is greater than the command end time
	return currentTime > commandEndTime;
}