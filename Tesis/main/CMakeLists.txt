idf_component_register(
        SRCS    "main.c"
                "wifi.c"
                "funciones_nodo/varias.c"
                "funciones_nodo/tareas.c"
                "funciones_nodo/file_server.c"
                "mpu_9250/acelerometroI2C.c"
                "funciones_nodo/GPIO.c"
                "sd_card/sd_card.c"
        INCLUDE_DIRS  "funciones_nodo" "sd_card"
        EMBED_FILES "funciones_nodo/favicon.ico" "funciones_nodo/upload_script.html")
