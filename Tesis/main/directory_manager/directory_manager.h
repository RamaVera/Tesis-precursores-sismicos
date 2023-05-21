//
// Created by Ramiro on 5/13/2023.
//

#ifndef DIR_MANAGER_H
#define DIR_MANAGER_H

#include "../sd_card/sd_card.h"

#define PERM_READ S_IRUSR
#define PERM_WRITE S_IWUSR
#define PERM_EXECUTION S_IXUSR
#define PERM_GROUP_READ S_IRGRP
#define PERM_GROUP_WRITE S_IWGRP
#define PERM_GROUP_EXECUTION S_IXGRP
#define PERM_OTHER_READ S_IROTH
#define PERM_OTHER_WRITE S_IWOTH
#define PERM_OTHER_EXECUTION S_IXOTH
#define PERM_ADMIN (PERM_READ | PERM_WRITE | PERM_EXECUTION | PERM_GROUP_READ | PERM_GROUP_WRITE | PERM_GROUP_EXECUTION |PERM_OTHER_READ | PERM_OTHER_WRITE | PERM_OTHER_EXECUTION)

#define SAMPLE_PATH MOUNT_POINT"/samples"

#define MAX_SAMPLE_PATH_LENGTH 52

esp_err_t DIR_setMainSampleDirectory(int year, int month, int day);
esp_err_t DIR_CreateDirectory(char *directoryPath);
esp_err_t DIR_getPathToWrite(char * path);


#endif //DIR_MANAGER_H
