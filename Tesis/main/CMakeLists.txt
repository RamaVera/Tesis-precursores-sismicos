idf_component_register(SRCS "main.c" "wifi.c" "funciones_nodo/varias.c" "funciones_nodo/tareas.c" "funciones_nodo/file_server.c" "funciones_nodo/acelerometroI2C.c" "funciones_nodo/GPIO.c" "funciones_nodo/sd_card.c"
                    INCLUDE_DIRS  "funciones_nodo"
                    EMBED_FILES "funciones_nodo/favicon.ico" "funciones_nodo/upload_script.html")
