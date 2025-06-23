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


    // Cargar fuente TTF desde disco (colocar el .ttf en tu proyecto, p.ej. "resources/MyFont.ttf")
    //Font myFont = LoadFont("Roboto-Black.ttf");
//    if (myFont.texture.id == 0) {
        // No se cargó: maneja error o usa fuente por defecto
        TraceLog(LOG_WARNING, "No se pudo cargar la fuente, se usará la predeterminada.");
//    } else {
        // Asignar la fuente a raygui
//        GuiSetFont(myFont);
        // Opcional: también puedes usar DrawTextEx con myFont directamente cuando dibujes textos propios:
        // DrawTextEx(myFont, "Hola Mundo", (Vector2){100,100}, 20, 1, BLACK);
	//  }
    
    SetTargetFPS(60);

    // Inicializar pantallas
    InitScreens();

    // Bucle principal
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Dibuja la pantalla actual (internamente hace switch según screen)
        DrawCurrentScreen();

        EndDrawing();
    }

    // Liberar recursos y cerrar
    UnloadScreens();
    CloseWindow();
    db_close();

    return 0;
}

