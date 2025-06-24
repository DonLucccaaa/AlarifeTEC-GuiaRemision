#include <stdio.h>
#include "raylib.h"
#include "db.h"
#include "screens.h"
#include "raygui.h"

int main(void) {
    // Abrir y inicializar base de datos
    if (!db_open("guia_remision.db")) {
        fprintf(stderr, "Error abriendo base de datos\n");
        return 1;
    }
    if (!db_init_schema()) {
        fprintf(stderr, "Error inicializando esquema de BD\n");
        db_close();
        return 1;
    }

    // Pre-poblar roles y tipo_envio si vacíos
    db_exec("INSERT OR IGNORE INTO Mae_rol(nombre) VALUES('Admin');");
    db_exec("INSERT OR IGNORE INTO Mae_rol(nombre) VALUES('Encargado');");
    db_exec("INSERT OR IGNORE INTO Mae_rol(nombre) VALUES('Consultor');");

    db_exec("INSERT OR IGNORE INTO Mae_tipo_envio(tipo) VALUES('Terrestre');");
    db_exec("INSERT OR IGNORE INTO Mae_tipo_envio(tipo) VALUES('Aéreo');");
    db_exec("INSERT OR IGNORE INTO Mae_tipo_envio(tipo) VALUES('Marítimo');");

    // Pre-poblar empresa con id 1 si no existe
    db_exec("INSERT OR IGNORE INTO Mae_empresa(id_empresa, nombre) VALUES(1, 'MiEmpresa');");

    // Inicializar ventana
    InitWindow(800, 600, "Guía de Remisión");
    
    SetTargetFPS(60);

    // Inicializar pantallas
    InitScreens();

    // Bucle principal
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Dibuja la pantalla actual
        DrawCurrentScreen();

        EndDrawing();
    }

    // Liberar recursos 
    UnloadScreens();
    CloseWindow();
    db_close();

    return 0;
}

