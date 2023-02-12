/** \file	mqtt.h
 *  \brief	Contiene las funciones de inicializacion y manejo del protocolo mqtt
 *  Autor: Ramiro Alonso
 *  Versión: 1
 *	Contiene las funciones de manejo e inicializacion de los GPIOs
 */


#ifndef FILE_SERVER_H
#define FILE_SERVER_H

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "esp_err.h"
#include "esp_log.h"

#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_http_server.h"
#include "../main.h"

// static esp_err_t index_html_get_handler(httpd_req_t *req);
//
// static esp_err_t favicon_get_handler(httpd_req_t *req);
//
// static esp_err_t http_resp_dir_html(httpd_req_t *req, const char *dirpath);
//
// static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename);
//
// static const char* get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize);
//
// static esp_err_t download_get_handler(httpd_req_t *req);
//
// static esp_err_t upload_post_handler(httpd_req_t *req);
//
// static esp_err_t delete_post_handler(httpd_req_t *req);

esp_err_t start_file_server(const char *base_path);

#endif //FILE_SERVER_H
