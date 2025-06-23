// screens.c

#include <stdlib.h>     // atoi, malloc/free
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>    // sqlite3_last_insert_rowid
#include "raylib.h"

// Incluir implementación de raygui aquí (header-only):
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "auth.h"
#include "db.h"
#include "utils.h"
#include "screens.h"
#include "pdf_gen.h"
#include <limits.h>


// Variable estática interna que guarda la pantalla actual
static int screen = 0;
static int current_user = 0;
static int current_role = 0;
static int current_session_id = 0;

// Funciones para gestionar la pantalla actual
void SetScreen(int s) {
    screen = s;
}
int GetScreen(void) {
    return screen;
}

// Función que dibuja la pantalla según el valor de 'screen'
void DrawCurrentScreen(void) {
    switch (screen) {
        case 0: ShowScreen_Login(); break;
        case 1: ShowScreen_Register(); break;
        case 2: ShowScreen_MainMenu(); break;
        case 3: ShowScreen_AddCliente(); break;
        case 4: ShowScreen_AddTransportista(); break;
        case 5: ShowScreen_UpdateEmpresa(); break;
        case 6: ShowScreen_GuiaRemision(); break;
        case 7: ShowScreen_QueryGuias(); break;
        case 8: ShowScreen_Warning(); break;
        default: ShowScreen_Login(); break;
    }
}

void InitScreens() {
    // Si necesitas inicializar recursos compartidos de pantallas (texturas, fuentes, etc.)
}

void UnloadScreens() {
    // Liberar recursos si aplica
}

// ------------------- Pantallas -------------------

void ShowScreen_Login() {
    static char user[32] = "";
    static char pass[32] = "";        // almacena la contraseña real
    static char passMask[32] = "";    // para mostrar '*' en pantalla
    static bool editUser = false;
    static bool editPass = false;
    static char msg[64] = "";

    DrawText("Login", 20, 20, 20, BLACK);

    // Campo Usuario
    GuiLabel((Rectangle){20, 60, 100, 20}, "Usuario:");
    if (GuiTextBox((Rectangle){130, 60, 200, 20}, user, 31, editUser)) {
        editUser = !editUser;
    }

    // Campo Contraseña: enmascarado
    GuiLabel((Rectangle){20, 100, 100, 20}, "Contra:");
    Rectangle passRect = (Rectangle){130, 100, 200, 20};
    // Dibujar fondo de TextBox:
    DrawRectangleRec(passRect, LIGHTGRAY);
    DrawRectangleLines((int)passRect.x, (int)passRect.y, (int)passRect.width, (int)passRect.height, DARKGRAY);
    // Mostrar passMask
    DrawText(passMask, (int)passRect.x + 5, (int)passRect.y + 2, 14, BLACK);

    // Detectar click para alternar edición
    Vector2 mouse = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mouse, passRect)) {
            editPass = true;
        } else {
            editPass = false;
        }
    }
    // Si está en edición, capturar caracteres
    if (editPass) {
        int key = GetCharPressed();
        while (key > 0) {
            // Sólo caracteres imprimibles
            if (key >= 32 && key <= 125 && (int)strlen(pass) < 31) {
                // Append al buffer real
                int len = strlen(pass);
                pass[len] = (char)key;
                pass[len+1] = '\0';
                // Actualizar máscara
                passMask[len] = '*';
                passMask[len+1] = '\0';
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && strlen(pass) > 0) {
            int len = strlen(pass);
            pass[len-1] = '\0';
            passMask[len-1] = '\0';
        }
    }

    // Botón Ingresar: validar usuario y pass no vacíos
    if (GuiButton((Rectangle){130, 140, 100, 30}, "Ingresar")) {
        if (!validar_texto(user, 31) || !validar_texto(pass, 31)) {
            strncpy(msg, "Usuario y contraseña obligatorios", sizeof(msg)-1);
            msg[sizeof(msg)-1] = '\0';
        } else {
            int uid, rid;
            if (login_check(user, pass, &uid, &rid)) {
                current_user = uid;
                current_role = rid;
                start_session(uid);
                current_session_id = (int)sqlite3_last_insert_rowid(db);
                log_auditoria(current_user, "Login exitoso", "Mae_usuario");
                msg[0] = '\0';
                // Navegar a menú principal
                SetScreen(2);
                // Limpiar campos
                user[0] = pass[0] = passMask[0] = '\0';
            } else {
                strncpy(msg, "Usuario o contraseña inválidos", sizeof(msg)-1);
                msg[sizeof(msg)-1] = '\0';
            }
        }
    }

    // Botón Crear cuenta
    if (GuiButton((Rectangle){250, 140, 120, 30}, "Crear cuenta")) {
        msg[0] = '\0';
        SetScreen(1);
        user[0] = pass[0] = passMask[0] = '\0';
    }

    if (msg[0] != '\0') {
        DrawText(msg, 20, 180, 14, RED);
    }
}



void ShowScreen_Register() {
    static char user[32] = "";
    static char email[64] = "";
    static char pass[32] = "";
    static char code[32] = "";
    static char msg[128] = "";
    static int role_sel = 0; // 0 => Encargado, 1 => Consultor

    static bool editUser = false;
    static bool editEmail = false;
    static bool editPass = false;
    static bool editCode = false;

    DrawText("Registro", 20, 20, 20, BLACK);

    // Usuario: sólo letras y números permitidos? A menudo se permiten letras, dígitos, guiones. Aquí validamos nombre sin espacios y alfanumérico mínimo:
    GuiLabel((Rectangle){20, 60, 100, 20}, "Usuario:");
    if (GuiTextBox((Rectangle){130, 60, 200, 20}, user, 31, editUser)) editUser = !editUser;

    // Correo
    GuiLabel((Rectangle){20, 100, 100, 20}, "Correo:");
    if (GuiTextBox((Rectangle){130, 100, 300, 20}, email, 63, editEmail)) editEmail = !editEmail;

    // Contraseña enmascarada como en login
    GuiLabel((Rectangle){20, 140, 100, 20}, "Contra:");
    Rectangle passRect = (Rectangle){130, 140, 200, 20};
    static char passMask[32] = "";
    DrawRectangleRec(passRect, LIGHTGRAY);
    DrawRectangleLines((int)passRect.x, (int)passRect.y, (int)passRect.width, (int)passRect.height, DARKGRAY);
    DrawText(passMask, (int)passRect.x + 5, (int)passRect.y + 2, 14, BLACK);
    Vector2 mouse = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mouse, passRect)) {
            editPass = true;
        } else {
            editPass = false;
        }
    }
    if (editPass) {
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 125 && (int)strlen(pass) < 31) {
                int len = strlen(pass);
                pass[len] = (char)key;
                pass[len+1] = '\0';
                passMask[len] = '*';
                passMask[len+1] = '\0';
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && strlen(pass) > 0) {
            int len = strlen(pass);
            pass[len-1] = '\0';
            passMask[len-1] = '\0';
        }
    }
    
    // Selección de rol
    GuiLabel((Rectangle){20, 220, 100, 20}, "Rol:");
    const char *roleOptions[2] = {"Encargado","Consultor"};
    // Si tu versión de raygui tiene GuiComboBox:
    //if (GuiComboBox((Rectangle){130, 180, 200, 20}, roleOptions[role_sel], &role_sel, 2)) {
        // Internamente maneja la apertura y selección
    //}
    // Si no, podrías usar GuiDropdownBox con &role_sel y un flag editRole:
     static bool editRole = false;
    if (GuiDropdownBox((Rectangle){130, 220, 200, 20}, "Encargado\nConsultor", &role_sel, editRole)) {
         editRole = !editRole;
    }

    // Código Admin
    GuiLabel((Rectangle){20, 180, 100, 20}, "Codigo Admin:");
    if (GuiTextBox((Rectangle){130, 180, 200, 20}, code, 31, editCode)) editCode = !editCode;

    // Botón Solicitar código
    if (GuiButton((Rectangle){130, 400, 100, 30}, "Solicitar codigo")) {
        strncpy(msg, "Contacte al admin para obtener código", sizeof(msg)-1);
        msg[sizeof(msg)-1] = '\0';
    }

    // Botón Registrar
    if (GuiButton((Rectangle){250, 400, 100, 30}, "Registrar")) {
        // Validaciones:
        if (!validar_nombre(user, 31)) {
            strncpy(msg, "Usuario inválido (solo letras y espacios, <=31)", sizeof(msg)-1);
        }
        else if (!validar_email(email, 63)) {
            strncpy(msg, "Email inválido", sizeof(msg)-1);
        }
        else if (!validar_texto(pass, 31)) {
            strncpy(msg, "Contraseña obligatoria (<=31)", sizeof(msg)-1);
        }
        else if (!validar_texto(code, 31)) {
            strncpy(msg, "Código Admin obligatorio", sizeof(msg)-1);
        }
        else if (!check_admin_code(code)) {
            strncpy(msg, "Código Admin inválido", sizeof(msg)-1);
        }
        else {
            int role_id = (role_sel == 0 ? ROLE_ENCARGADO : ROLE_CONSULTOR);
            if (db_create_user(user, email, pass, role_id)) {
                log_auditoria(-1, "Registro usuario", "Mae_usuario");
                strncpy(msg, "Registrado con éxito. Inicie sesión.", sizeof(msg)-1);
                
            } else {
                strncpy(msg, "Error: usuario o correo ya existe", sizeof(msg)-1);
            }
        }

	    
        msg[sizeof(msg)-1] = '\0';
    }

    // Botón Volver
    if (GuiButton((Rectangle){20, 400, 80, 30}, "Volver")) {
        user[0] = email[0] = pass[0] = passMask[0] = code[0] = msg[0] = '\0';
        editUser = editEmail = editPass = editCode = false;
        SetScreen(0);
    }

    if (msg[0] != '\0') {
        DrawText(msg, 20, 260, 14, RED);
    }
}


void ShowScreen_MainMenu()
{
       DrawText("Menú Principal", 20, 20, 20, BLACK);

    if (current_role == ROLE_ADMIN) {
        if (GuiButton((Rectangle){20, 60, 200, 30}, "Agregar Cliente")) SetScreen(3);
        if (GuiButton((Rectangle){20, 100, 200, 30}, "Agregar Transportista")) SetScreen(4);
        if (GuiButton((Rectangle){20, 140, 200, 30}, "Actualizar Empresa")) SetScreen(5);
        if (GuiButton((Rectangle){20, 180, 200, 30}, "Registrar Guía")) SetScreen(6);
        if (GuiButton((Rectangle){20, 220, 200, 30}, "Consultar Guías")) SetScreen(7);
    }
    else if (current_role == ROLE_ENCARGADO) {
        if (GuiButton((Rectangle){20, 60, 200, 30}, "Agregar Cliente")) SetScreen(3);
        if (GuiButton((Rectangle){20, 100, 200, 30}, "Agregar Transportista")) SetScreen(4);
        // No “Actualizar Empresa”
        if (GuiButton((Rectangle){20, 140, 200, 30}, "Registrar Guía")) SetScreen(6);
        if (GuiButton((Rectangle){20, 180, 200, 30}, "Consultar Guías")) SetScreen(7);
    }
    else if (current_role == ROLE_CONSULTOR) {
        if (GuiButton((Rectangle){20, 60, 200, 30}, "Consultar Guías")) SetScreen(7);
    }

    // Botón Cerrar Sesión siempre visible
    int ySalir = 300;
    if (GuiButton((Rectangle){20, ySalir, 200, 30}, "Cerrar Sesión")) {
        end_session(current_session_id);
        current_user = 0;
        current_role = 0;
        SetScreen(0);
    }

}


void ShowScreen_AddCliente() {
    static char razon[64] = "";
    static char ruc[16] = "";
    static char dir[128] = "";
    static char correo[64] = "";
    static char tel[16] = "";
    static char msg[128] = "";

    static bool editRazon = false;
    static bool editRuc = false;
    static bool editDir = false;
    static bool editCorreo = false;
    static bool editTel = false;

    DrawText("Agregar Cliente", 20, 20, 20, BLACK);

    // Razon social: permitimos letras, espacios y quizás dígitos? A menudo puede incluir números o caracteres especiales. 
    // Si estrictamente sólo letras y espacios:
    GuiLabel((Rectangle){20, 60, 100, 20}, "Razon:");
    if (GuiTextBox((Rectangle){130, 60, 300, 20}, razon, 63, editRazon)) editRazon = !editRazon;

    // RUC: sólo dígitos, longitud exacta 11
    GuiLabel((Rectangle){20, 100, 100, 20}, "RUC:");
    if (GuiTextBox((Rectangle){130, 100, 200, 20}, ruc, 15, editRuc)) editRuc = !editRuc;

    // Dirección: aquí es dirección web? Si es dirección fiscal física, no tiene formato www; 
    // según tu requerimiento mencionaste formato www.ejemplo.com:
    GuiLabel((Rectangle){20, 140, 100, 20}, "Dir:");
    if (GuiTextBox((Rectangle){130, 140, 400, 20}, dir, 127, editDir)) editDir = !editDir;

    // Correo
    GuiLabel((Rectangle){20, 180, 100, 20}, "Correo:");
    if (GuiTextBox((Rectangle){130, 180, 300, 20}, correo, 63, editCorreo)) editCorreo = !editCorreo;

    // Teléfono: sólo dígitos
    GuiLabel((Rectangle){20, 220, 100, 20}, "Telefono:");
    if (GuiTextBox((Rectangle){130, 220, 200, 20}, tel, 15, editTel)) editTel = !editTel;

    if (GuiButton((Rectangle){130, 260, 100, 30}, "Agregar")) {
        // Validaciones:
        if (!validar_texto(razon, 63)) {
            strncpy(msg, "Razon social obligatoria (<=63)", sizeof(msg)-1);
        }
        else if (!validar_ruc(ruc)) {
            strncpy(msg, "RUC inválido (11 dígitos)", sizeof(msg)-1);
        }
//        else if (!validar_url(dir, 127)) {
//            strncpy(msg, "Dirección inválida (formato URL)", sizeof(msg)-1);
//        }
        else if (!validar_email(correo, 63)) {
            strncpy(msg, "Correo inválido", sizeof(msg)-1);
        }
        else if (!validar_telefono(tel)) {
            strncpy(msg, "Teléfono inválido (solo dígitos, longitud 7-15)", sizeof(msg)-1);
        }
        else {
            if (db_add_cliente(razon, ruc, dir, correo, tel)) {
                log_auditoria(current_user, "Agregar cliente", "Mae_cliente");
                strncpy(msg, "Cliente agregado exitosamente", sizeof(msg)-1);
                // Limpiar campos
                razon[0] = ruc[0] = dir[0] = correo[0] = tel[0] = '\0';
            } else {
                strncpy(msg, "Error al agregar cliente (DB)", sizeof(msg)-1);
            }
        }
        msg[sizeof(msg)-1] = '\0';
    }
    if (GuiButton((Rectangle){20, 260, 80, 30}, "Volver")) {
        msg[0] = '\0';
        razon[0] = ruc[0] = dir[0] = correo[0] = tel[0] = '\0';
        editRazon = editRuc = editDir = editCorreo = editTel = false;
        SetScreen(2);
    }

    if (msg[0] != '\0') {
        DrawText(msg, 20, 300, 14, RED);
    }
}


void ShowScreen_AddTransportista() {
    static char nombre[64] = "";
    static char licencia[32] = "";
    static char telefono[16] = "";
    static char placa[16] = "";
    static char marca[32] = "";
    static char modelo[32] = "";
    static char anio_str[8] = "";
    static char msg[128] = "";

    static bool editNombre = false;
    static bool editLicencia = false;
    static bool editTelefono = false;
    static bool editPlaca = false;
    static bool editMarca = false;
    static bool editModelo = false;
    static bool editAnio = false;

    DrawText("Agregar Transportista", 20, 20, 20, BLACK);

    // Nombre: sólo letras y espacios (si deseas permitir apellidos con espacios)
    GuiLabel((Rectangle){20, 60, 100, 20}, "Nombre:");
    if (GuiTextBox((Rectangle){130, 60, 300, 20}, nombre, 63, editNombre)) editNombre = !editNombre;

    // Licencia: alfanumérico, validación mínima: no vacío
    GuiLabel((Rectangle){20, 100, 100, 20}, "Licencia:");
    if (GuiTextBox((Rectangle){130, 100, 200, 20}, licencia, 31, editLicencia)) editLicencia = !editLicencia;

    // Teléfono
    GuiLabel((Rectangle){20, 140, 100, 20}, "Telefono:");
    if (GuiTextBox((Rectangle){130, 140, 200, 20}, telefono, 15, editTelefono)) editTelefono = !editTelefono;

    DrawText("Vehiculo", 20, 180, 20, BLACK);

    // Placa: formato depende (alfanumérico, longitud 6-8)
    GuiLabel((Rectangle){20, 210, 100, 20}, "Placa:");
    if (GuiTextBox((Rectangle){130, 210, 200, 20}, placa, 15, editPlaca)) editPlaca = !editPlaca;

    GuiLabel((Rectangle){20, 250, 100, 20}, "Marca:");
    if (GuiTextBox((Rectangle){130, 250, 200, 20}, marca, 31, editMarca)) editMarca = !editMarca;

    GuiLabel((Rectangle){20, 290, 100, 20}, "Modelo:");
    if (GuiTextBox((Rectangle){130, 290, 200, 20}, modelo, 31, editModelo)) editModelo = !editModelo;

    GuiLabel((Rectangle){20, 330, 100, 20}, "Anio:");
    if (GuiTextBox((Rectangle){130, 330, 100, 20}, anio_str, 7, editAnio)) editAnio = !editAnio;

    if (GuiButton((Rectangle){130, 370, 100, 30}, "Agregar")) {
        // Validaciones:
        if (!validar_nombre(nombre, 63)) {
            strncpy(msg, "Nombre inválido (solo letras y espacios)", sizeof(msg)-1);
        }
        else if (!validar_licencia(licencia)) {
            strncpy(msg, "Licencia inválida: 1 letra + 8 dígitos", sizeof(msg)-1);
        }
        else if (!validar_telefono(telefono)) {
            strncpy(msg, "Teléfono inválido", sizeof(msg)-1);
        }
        else if (!validar_placa(placa)) {
            strncpy(msg, "Placa inválida: debe tener 6 dígitos", sizeof(msg)-1);
        }
        else if (!validar_texto(marca, 31)) {
            strncpy(msg, "Marca obligatoria", sizeof(msg)-1);
        }
        else if (!validar_texto(modelo, 31)) {
            strncpy(msg, "Modelo obligatorio", sizeof(msg)-1);
        }
        else if (!validar_anio(anio_str)) {
            strncpy(msg, "Año inválido (1900-" /*se completa abajo*/ ")", sizeof(msg)-1);
            // Nota: validar_anio ya checkea rango
        }
        else {
            int trans_id;
            if (db_add_transportista(nombre, licencia, telefono, &trans_id)) {
                int anio = atoi(anio_str);
                if (db_add_vehiculo(placa, marca, modelo, anio, trans_id)) {
                    log_auditoria(current_user, "Agregar transportista+vehiculo", "Mae_transportista/Mae_vehiculo");
                    strncpy(msg, "Transportista y vehículo agregados", sizeof(msg)-1);
                    // Limpiar campos
                    nombre[0] = licencia[0] = telefono[0] = placa[0] = marca[0] = modelo[0] = anio_str[0] = '\0';
                } else {
                    strncpy(msg, "Error al agregar vehículo", sizeof(msg)-1);
                }
            } else {
                strncpy(msg, "Error al agregar transportista", sizeof(msg)-1);
            }
        }
        msg[sizeof(msg)-1] = '\0';
    }
    if (GuiButton((Rectangle){20, 370, 80, 30}, "Volver")) {
        msg[0] = '\0';
        nombre[0] = licencia[0] = telefono[0] = placa[0] = marca[0] = modelo[0] = anio_str[0] = '\0';
        editNombre = editLicencia = editTelefono = editPlaca = editMarca = editModelo = editAnio = false;
        SetScreen(2);
    }

    if (msg[0] != '\0') {
        DrawText(msg, 20, 410, 14, RED);
    }
}


void ShowScreen_UpdateEmpresa() {
    static char razon[64] = "";
    static char ruc[16] = "";
    static char dir[128] = "";
    static char web[64] = "";
    static char correo[64] = "";
    static char tel[16] = "";
    static char lema[128] = "";
    static bool loaded = false;
    static char msg[128] = "";

    static bool editRazon = false;
    static bool editRuc = false;
    static bool editDir = false;
    static bool editWeb = false;
    static bool editCorreo = false;
    static bool editTel = false;
    static bool editLema = false;

    DrawText("Actualizar Empresa", 20, 20, 20, BLACK);

    if (!loaded) {
        if (db_get_empresa(1, razon, ruc, dir, web, correo, tel, lema)) {
            // datos cargados
        }
        loaded = true;
    }

    GuiLabel((Rectangle){20, 60, 100, 20}, "Razon:");
    if (GuiTextBox((Rectangle){130, 60, 300, 20}, razon, 63, editRazon)) editRazon = !editRazon;

    GuiLabel((Rectangle){20, 100, 100, 20}, "RUC:");
    if (GuiTextBox((Rectangle){130, 100, 200, 20}, ruc, 15, editRuc)) editRuc = !editRuc;

    GuiLabel((Rectangle){20, 140, 100, 20}, "Dir:");
    if (GuiTextBox((Rectangle){130, 140, 400, 20}, dir, 127, editDir)) editDir = !editDir;

    GuiLabel((Rectangle){20, 180, 100, 20}, "Web:");
    if (GuiTextBox((Rectangle){130, 180, 300, 20}, web, 63, editWeb)) editWeb = !editWeb;

    GuiLabel((Rectangle){20, 220, 100, 20}, "Correo:");
    if (GuiTextBox((Rectangle){130, 220, 300, 20}, correo, 63, editCorreo)) editCorreo = !editCorreo;

    GuiLabel((Rectangle){20, 260, 100, 20}, "Telefono:");
    if (GuiTextBox((Rectangle){130, 260, 200, 20}, tel, 15, editTel)) editTel = !editTel;

    GuiLabel((Rectangle){20, 300, 100, 20}, "Lema:");
    if (GuiTextBox((Rectangle){130, 300, 400, 20}, lema, 127, editLema)) editLema = !editLema;

    if (GuiButton((Rectangle){130, 340, 100, 30}, "Aceptar")) {
        // Validaciones:
        if (!validar_texto(razon, 63)) {
            strncpy(msg, "Razon obligatoria", sizeof(msg)-1);
        }
        else if (!validar_ruc(ruc)) {
            strncpy(msg, "RUC inválido (11 dígitos)", sizeof(msg)-1);
        }
//        else if (!validar_url(dir, 127)) {
//            strncpy(msg, "Dirección inválida (URL)", sizeof(msg)-1);
//        }
        else if (!validar_url(web, 63)) {
            strncpy(msg, "Web inválida (URL)", sizeof(msg)-1);
        }
        else if (!validar_email(correo, 63)) {
            strncpy(msg, "Email inválido", sizeof(msg)-1);
        }
        else if (!validar_telefono(tel)) {
            strncpy(msg, "Teléfono inválido", sizeof(msg)-1);
        }
        else if (!validar_texto(lema, 127)) {
            strncpy(msg, "Lema obligatorio", sizeof(msg)-1);
        }
        else {
            if (db_update_empresa(1, razon, ruc, dir, web, correo, tel, lema)) {
                log_auditoria(current_user, "Actualizar empresa", "Mae_empresa");
                strncpy(msg, "Datos de empresa actualizados", sizeof(msg)-1);
            } else {
                strncpy(msg, "Error al actualizar empresa", sizeof(msg)-1);
            }
        }
        msg[sizeof(msg)-1] = '\0';
    }

    if (GuiButton((Rectangle){20, 340, 80, 30}, "Volver")) {
        msg[0] = '\0';
        loaded = false;
        editRazon = editRuc = editDir = editWeb = editCorreo = editTel = editLema = false;
        SetScreen(2);
    }

    if (msg[0] != '\0') {
        DrawText(msg, 20, 380, 14, RED);
    }
}

void ShowScreen_GuiaRemision() {
    DrawText("Registrar Guía de Remisión", 20, 20, 20, BLACK);

    // Campos de texto estáticos
    static char fecha_emision[12] = "";
    static char fecha_inicio[12] = "";
    static char punto_partida[128] = "";
    static char punto_llegada[128] = "";
    static char motivo[64] = "";
    static char estado[32] = "";
    static char descripcion[256] = "";

    // Selecciones de dropdown
    static int selected_cliente = 0;
    static int selected_transportista = 0;
    static int selected_vehiculo = 0;
    static int selected_tipo_envio = 0;

    // Listas y arrays de IDs/labels (se asume que cargar_clientes, etc. llenan estos)
    static char **clientes = NULL;
    static int *clientes_ids = NULL;
    static int clientes_count = 0;
    static char *clientes_items_str = NULL;

    static char **transportistas = NULL;
    static int *transportistas_ids = NULL;
    static int transportistas_count = 0;
    static char *transportistas_items_str = NULL;

    static char **vehiculos = NULL;
    static int *vehiculos_ids = NULL;
    static int vehiculos_count = 0;
    static char *vehiculos_items_str = NULL;

    static char **tipos_envio = NULL;
    static int *tipos_envio_ids = NULL;
    static int tipos_envio_count = 0;
    static char *tipos_envio_items_str = NULL;

    // Flags de carga única
    static bool loaded = false;

    // Flags de edición (expansión) para cada dropdown
    static bool editCliente = false;
    static bool editTransportista = false;
    static bool editVehiculo = false;
    static bool editTipoEnvio = false;

    // Flags de edición para TextBox
    static bool editFechaEmi = false, editFechaIni = false;
    static bool editPartida = false, editLlegada = false, editMotivo = false, editEstado = false, editDesc = false;

    // ---------- Variables para lista de detalles ----------
    typedef struct {
        char producto[128];
        char cantidad_str[16];
        char valor_unitario_str[32];
        char descripcion[256];
    } DetalleTmp;

    static DetalleTmp *detalles = NULL;
    static int detalles_count = 0;
    static int detalles_cap = 0;
    // Flags de edición para campos temporales de detalle
    static bool editProd = false;
    static bool editCant = false;
    static bool editVal = false;
    static bool editDescTmp = false;
    // Campos temporales para nuevo detalle
    static char producto_tmp[128] = "";
    static char cantidad_tmp[16] = "";
    static char valor_tmp[32] = "";
    static char descripcion_tmp[256] = "";

    static char msg[256] = "";

    // Carga de listas solo una vez
    if (!loaded) {
        // Cargar clientes
        cargar_clientes(&clientes, &clientes_ids, &clientes_count);
        if (clientes_count > 0) {
            int total_len = 0;
            for (int i = 0; i < clientes_count; i++) total_len += strlen(clientes[i]) + 1;
            clientes_items_str = malloc(total_len + 1);
            if (clientes_items_str) {
                clientes_items_str[0] = '\0';
                for (int i = 0; i < clientes_count; i++) {
                    strcat(clientes_items_str, clientes[i]);
                    if (i < clientes_count - 1) strcat(clientes_items_str, "\n");
                }
            }
        }
        // Cargar transportistas
        cargar_transportistas(&transportistas, &transportistas_ids, &transportistas_count);
        if (transportistas_count > 0) {
            int total_len = 0;
            for (int i = 0; i < transportistas_count; i++) total_len += strlen(transportistas[i]) + 1;
            transportistas_items_str = malloc(total_len + 1);
            if (transportistas_items_str) {
                transportistas_items_str[0] = '\0';
                for (int i = 0; i < transportistas_count; i++) {
                    strcat(transportistas_items_str, transportistas[i]);
                    if (i < transportistas_count - 1) strcat(transportistas_items_str, "\n");
                }
            }
        }
        // Cargar vehículos
        cargar_vehiculos(&vehiculos, &vehiculos_ids, &vehiculos_count);
        if (vehiculos_count > 0) {
            int total_len = 0;
            for (int i = 0; i < vehiculos_count; i++) total_len += strlen(vehiculos[i]) + 1;
            vehiculos_items_str = malloc(total_len + 1);
            if (vehiculos_items_str) {
                vehiculos_items_str[0] = '\0';
                for (int i = 0; i < vehiculos_count; i++) {
                    strcat(vehiculos_items_str, vehiculos[i]);
                    if (i < vehiculos_count - 1) strcat(vehiculos_items_str, "\n");
                }
            }
        }
        // Cargar tipos de envío
        cargar_tipos_envio(&tipos_envio, &tipos_envio_ids, &tipos_envio_count);
        if (tipos_envio_count > 0) {
            int total_len = 0;
            for (int i = 0; i < tipos_envio_count; i++) total_len += strlen(tipos_envio[i]) + 1;
            tipos_envio_items_str = malloc(total_len + 1);
            if (tipos_envio_items_str) {
                tipos_envio_items_str[0] = '\0';
                for (int i = 0; i < tipos_envio_count; i++) {
                    strcat(tipos_envio_items_str, tipos_envio[i]);
                    if (i < tipos_envio_count - 1) strcat(tipos_envio_items_str, "\n");
                }
            }
        }
        loaded = true;
    }

    // Campo Fecha Emisión
    GuiLabel((Rectangle){20, 60, 120, 20}, "Fecha emisión:");
    if (GuiTextBox((Rectangle){150, 60, 100, 20}, fecha_emision, 11, editFechaEmi)) editFechaEmi = !editFechaEmi;
    DrawText("(YYYY-MM-DD)", 260, 60, 12, DARKGRAY);

    // Campo Fecha Inicio Traslado
    GuiLabel((Rectangle){20, 90, 120, 20}, "Fecha inicio:");
    if (GuiTextBox((Rectangle){150, 90, 100, 20}, fecha_inicio, 11, editFechaIni)) editFechaIni = !editFechaIni;
    DrawText("(YYYY-MM-DD)", 260, 90, 12, DARKGRAY);

    // --- Dropdown Layout (Labels Above, All Four Horizontally) ---
    const int labelHeight = 20;
    const int dropdownWidth = 150; // Width of each dropdown box
    const int controlHeight = 20;  // Height of each dropdown box
    const int verticalGapBetweenLabelAndDropdown = 5; // Gap between label and its dropdown
    const int horizontalGroupSpacing = 30; // Space between each label-dropdown group
    const int dropdownRowY = 130; // Y position for the start of the dropdown groups

    // Calculate total height for one label-dropdown group (used for yBase)
    const int dropdownGroupTotalHeight = labelHeight + verticalGapBetweenLabelAndDropdown + controlHeight;

    int currentX = 20; // Starting X position for the first group

    // Cliente Group
    GuiLabel((Rectangle){currentX, dropdownRowY, dropdownWidth, labelHeight}, "Cliente:");
    if (clientes_count > 0 && clientes_items_str) {
        if (GuiDropdownBox((Rectangle){currentX, dropdownRowY + labelHeight + verticalGapBetweenLabelAndDropdown, dropdownWidth, controlHeight}, clientes_items_str, &selected_cliente, editCliente)) {
            editCliente = !editCliente;
            if (editCliente) {
                editTransportista = false;
                editVehiculo = false;
                editTipoEnvio = false;
            }
        }
    }
    currentX += dropdownWidth + horizontalGroupSpacing;

    // Vehículo Group
    GuiLabel((Rectangle){currentX, dropdownRowY, dropdownWidth, labelHeight}, "Vehículo:");
    if (vehiculos_count > 0 && vehiculos_items_str) {
        if (GuiDropdownBox((Rectangle){currentX, dropdownRowY + labelHeight + verticalGapBetweenLabelAndDropdown, dropdownWidth, controlHeight}, vehiculos_items_str, &selected_vehiculo, editVehiculo)) {
            editVehiculo = !editVehiculo;
            if (editVehiculo) {
                editCliente = false;
                editTransportista = false;
                editTipoEnvio = false;
            }
        }
    }
    currentX += dropdownWidth + horizontalGroupSpacing;

    // Transportista Group
    GuiLabel((Rectangle){currentX, dropdownRowY, dropdownWidth, labelHeight}, "Transportista:");
    if (transportistas_count > 0 && transportistas_items_str) {
        if (GuiDropdownBox((Rectangle){currentX, dropdownRowY + labelHeight + verticalGapBetweenLabelAndDropdown, dropdownWidth, controlHeight}, transportistas_items_str, &selected_transportista, editTransportista)) {
            editTransportista = !editTransportista;
            if (editTransportista) {
                editCliente = false;
                editVehiculo = false;
                editTipoEnvio = false;
            }
        }
    }
    currentX += dropdownWidth + horizontalGroupSpacing;

    // Tipo Envío Group
    GuiLabel((Rectangle){currentX, dropdownRowY, dropdownWidth, labelHeight}, "Tipo Envío:");
    if (tipos_envio_count > 0 && tipos_envio_items_str) {
        if (GuiDropdownBox((Rectangle){currentX, dropdownRowY + labelHeight + verticalGapBetweenLabelAndDropdown, dropdownWidth, controlHeight}, tipos_envio_items_str, &selected_tipo_envio, editTipoEnvio)) {
            editTipoEnvio = !editTipoEnvio;
            if (editTipoEnvio) {
                editCliente = false;
                editTransportista = false;
                editVehiculo = false;
            }
        }
    }
    // --- End Dropdown Layout ---

    // Ahora colocar los campos de texto siguientes más abajo
    const int verticalSpacingInputs = 30; // Spacing for vertical text fields
    int yBase = dropdownRowY + dropdownGroupTotalHeight + 50; // Ajusta espacio

    GuiLabel((Rectangle){20, yBase, 100, controlHeight}, "Partida:");
    if (GuiTextBox((Rectangle){150, yBase, 300, controlHeight}, punto_partida, 127, editPartida)) editPartida = !editPartida;

    GuiLabel((Rectangle){20, yBase + verticalSpacingInputs, 100, controlHeight}, "Llegada:");
    if (GuiTextBox((Rectangle){150, yBase + verticalSpacingInputs, 300, controlHeight}, punto_llegada, 127, editLlegada)) editLlegada = !editLlegada;

    GuiLabel((Rectangle){20, yBase + 2*verticalSpacingInputs, 100, controlHeight}, "Motivo:");
    if (GuiTextBox((Rectangle){150, yBase + 2*verticalSpacingInputs, 200, controlHeight}, motivo, 63, editMotivo)) editMotivo = !editMotivo;

    GuiLabel((Rectangle){20, yBase + 3*verticalSpacingInputs, 100, controlHeight}, "Estado:");
    if (GuiTextBox((Rectangle){150, yBase + 3*verticalSpacingInputs, 100, controlHeight}, estado, 31, editEstado)) editEstado = !editEstado;

    GuiLabel((Rectangle){20, yBase + 4*verticalSpacingInputs, 100, controlHeight}, "Descripción:");
    if (GuiTextBox((Rectangle){150, yBase + 4*verticalSpacingInputs, 400, controlHeight}, descripcion, 255, editDesc)) editDesc = !editDesc;

    // ---------- Campos para nuevo detalle ----------
/*    GuiLabel((Rectangle){20, yBase + 5*verticalSpacingInputs, 100, controlHeight}, "Producto:");
    if (GuiTextBox((Rectangle){130, yBase + 5*verticalSpacingInputs, 200, controlHeight}, producto_tmp, 127, editProd)) editProd = !editProd;

    GuiLabel((Rectangle){20, yBase + 6*verticalSpacingInputs, 100, controlHeight}, "Cantidad:");
    if (GuiTextBox((Rectangle){130, yBase + 6*verticalSpacingInputs, 100, controlHeight}, cantidad_tmp, 15, editCant)) editCant = !editCant;

    GuiLabel((Rectangle){250, yBase + 6*verticalSpacingInputs, 100, controlHeight}, "Valor Unit.:");
    if (GuiTextBox((Rectangle){350, yBase + 6*verticalSpacingInputs, 100, controlHeight}, valor_tmp, 31, editVal)) editVal = !editVal;

    GuiLabel((Rectangle){20, yBase + 7*verticalSpacingInputs, 100, controlHeight}, "Desc. Detalle:");
    if (GuiTextBox((Rectangle){130, yBase + 7*verticalSpacingInputs, 300, controlHeight}, descripcion_tmp, 255, editDescTmp)) editDescTmp = !editDescTmp;

    // Botón Agregar Detalle
    if (GuiButton((Rectangle){20, yBase + 8*verticalSpacingInputs, 120, 30}, "Agregar Detalle")) {*/
    // ---------- Campos para nuevo detalle en una sola línea ----------

    // Producto:
    GuiLabel((Rectangle){20, yBase + 5*verticalSpacingInputs, 60, controlHeight}, "Producto:");
    if (GuiTextBox((Rectangle){90, yBase + 5*verticalSpacingInputs, 50, controlHeight}, producto_tmp, 127, editProd)) editProd = !editProd;

// Cantidad:
    GuiLabel((Rectangle){150, yBase + 5*verticalSpacingInputs, 60, controlHeight}, "Cantidad:");
    if (GuiTextBox((Rectangle){220, yBase + 5*verticalSpacingInputs, 30, controlHeight}, cantidad_tmp, 15, editCant)) editCant = !editCant;

// Valor Unitario:
    GuiLabel((Rectangle){270, yBase + 5*verticalSpacingInputs, 100, controlHeight}, "Valor Unit.(S./):");
    if (GuiTextBox((Rectangle){370, yBase + 5*verticalSpacingInputs, 30, controlHeight}, valor_tmp, 31, editVal)) editVal = !editVal;

// Desc. Detalle:
    GuiLabel((Rectangle){430, yBase + 5*verticalSpacingInputs, 70, controlHeight}, "Desc.Detalle:");
    if (GuiTextBox((Rectangle){520, yBase + 5*verticalSpacingInputs, 120, controlHeight}, descripcion_tmp, 255, editDescTmp)) editDescTmp = !editDescTmp;

// Botón Agregar Detalle al final & de la misma fila:
    if (GuiButton((Rectangle){700, yBase + 5*verticalSpacingInputs, 60, controlHeight}, "Agregar")) {
        bool ok = true;
        // Validar producto
        if (strlen(producto_tmp) == 0) {
            strncpy(msg, "Producto obligatorio", sizeof(msg)-1); ok = false;
        }
        // Validar cantidad >0
        int cant = 0;
        if (ok) {
            if (strlen(cantidad_tmp) == 0) {
                strncpy(msg, "Cantidad obligatoria", sizeof(msg)-1); ok = false;
            } else {
                for (int i = 0; cantidad_tmp[i]; i++) {
                    if (!isdigit((unsigned char)cantidad_tmp[i])) {
                        strncpy(msg, "Cantidad inválida", sizeof(msg)-1); ok = false; break;
                    }
                }
                if (ok) {
                    cant = atoi(cantidad_tmp);
                    if (cant <= 0) { strncpy(msg, "Cantidad > 0", sizeof(msg)-1); ok = false; }
                }
            }
        }
        // Validar valor unitario >= 0
        double val = 0.0;
        if (ok) {
            if (strlen(valor_tmp) == 0) {
                strncpy(msg, "Valor unitario obligatorio", sizeof(msg)-1); ok = false;
            } else {
                char *p;
                val = strtod(valor_tmp, &p);
                if (p == valor_tmp || *p != '\0' || val < 0.0) {
                    strncpy(msg, "Valor unitario inválido", sizeof(msg)-1); ok = false;
                }
            }
        }
        if (ok) {
            if (detalles_count >= detalles_cap) {
                int nueva = detalles_cap == 0 ? 4 : detalles_cap * 2;
                DetalleTmp *tmp = realloc(detalles, sizeof(DetalleTmp) * nueva);
                if (!tmp) {
                    strncpy(msg, "Error memoria detalles", sizeof(msg)-1);
                    ok = false;
                } else {
                    detalles = tmp;
                    detalles_cap = nueva;
                }
            }
            if (ok) {
                // Copiar valores temporales a lista
                strncpy(detalles[detalles_count].producto, producto_tmp, sizeof(detalles[0].producto)-1);
                detalles[detalles_count].producto[sizeof(detalles[0].producto)-1] = '\0';
                strncpy(detalles[detalles_count].cantidad_str, cantidad_tmp, sizeof(detalles[0].cantidad_str)-1);
                detalles[detalles_count].cantidad_str[sizeof(detalles[0].cantidad_str)-1] = '\0';
                strncpy(detalles[detalles_count].valor_unitario_str, valor_tmp, sizeof(detalles[0].valor_unitario_str)-1);
                detalles[detalles_count].valor_unitario_str[sizeof(detalles[0].valor_unitario_str)-1] = '\0';
                strncpy(detalles[detalles_count].descripcion, descripcion_tmp, sizeof(detalles[0].descripcion)-1);
                detalles[detalles_count].descripcion[sizeof(detalles[0].descripcion)-1] = '\0';
                detalles_count++;
                // Limpiar temporales
                producto_tmp[0] = cantidad_tmp[0] = valor_tmp[0] = descripcion_tmp[0] = '\0';
                strncpy(msg, "Detalle agregado", sizeof(msg)-1);
            }
        }
        msg[sizeof(msg)-1] = '\0';
    }

    // Mostrar lista de detalles agregados
    int yDet = yBase + 9*verticalSpacingInputs;
    for (int i = 0; i < detalles_count; i++) {
        char linea[256];
        snprintf(linea, sizeof(linea), "%d: %s | %s | %s", i+1,
                 detalles[i].producto,
                 detalles[i].cantidad_str,
                 detalles[i].valor_unitario_str);
        DrawText(linea, 20, 400 + i*20, 12, BLACK);
    }

    // Botón Aceptar / Registrar Guía
    if (GuiButton((Rectangle){150, yBase + 10*verticalSpacingInputs, 120, 30}, "Registrar Guía")) {
        // Declarar variables de cabecera e IDs antes de bloques internos
        int new_guia_id = 0;
        int id_cliente = -1;
        int id_transportista = -1;
        int id_vehiculo = -1;
        int id_envio = -1;
        int id_emisor = 1;     // según tu lógica
        int id_receptor = -1;

        // Validaciones cabecera
        if (!validar_fecha_yyyy_mm_dd(fecha_emision)) {
            strncpy(msg, "Fecha emisión inválida", sizeof(msg)-1);
        }
        else if (!validar_fecha_yyyy_mm_dd(fecha_inicio)) {
            strncpy(msg, "Fecha inicio inválida", sizeof(msg)-1);
        }
        else {
            int cmp = comparar_fechas_yyyy_mm_dd(fecha_emision, fecha_inicio);
            if (cmp == INT_MIN) {
                strncpy(msg, "Error al analizar fechas", sizeof(msg)-1);
            }
            else if (cmp >= 0) {
                strncpy(msg, "Fecha emisión debe ser anterior a fecha inicio", sizeof(msg)-1);
            }
            else if (selected_cliente < 0 || selected_cliente >= clientes_count) {
                strncpy(msg, "Seleccione Cliente válido", sizeof(msg)-1);
            }
            else if (selected_transportista < 0 || selected_transportista >= transportistas_count) {
                strncpy(msg, "Seleccione Transportista válido", sizeof(msg)-1);
            }
            else if (selected_vehiculo < 0 || selected_vehiculo >= vehiculos_count) {
                strncpy(msg, "Seleccione Vehículo válido", sizeof(msg)-1);
            }
            else if (selected_tipo_envio < 0 || selected_tipo_envio >= tipos_envio_count) {
                strncpy(msg, "Seleccione Tipo Envío válido", sizeof(msg)-1);
            }
            else if (!validar_texto(punto_partida, 127)) {
                strncpy(msg, "Partida obligatoria", sizeof(msg)-1);
            }
            else if (!validar_texto(punto_llegada, 127)) {
                strncpy(msg, "Llegada obligatoria", sizeof(msg)-1);
            }
            else if (!validar_texto(motivo, 63)) {
                strncpy(msg, "Motivo obligatorio", sizeof(msg)-1);
            }
            else if (!validar_texto(estado, 31)) {
                strncpy(msg, "Estado obligatorio", sizeof(msg)-1);
            }
            else if (detalles_count == 0) {
                strncpy(msg, "Agrega al menos un detalle", sizeof(msg)-1);
            }
            else {
                // Asignar IDs de dropdowns
                id_cliente = clientes_ids[selected_cliente];
                id_transportista = transportistas_ids[selected_transportista];
                id_vehiculo = vehiculos_ids[selected_vehiculo];
                id_envio = tipos_envio_ids[selected_tipo_envio];
                id_receptor = id_cliente; // según tu lógica

                // Insertar cabecera de guía
                if (!db_add_guia(fecha_emision, fecha_inicio, motivo, punto_partida, punto_llegada,
                                 estado, descripcion,
                                 id_envio, id_cliente, id_emisor, id_receptor,
                                 id_transportista, id_vehiculo,
                                 &new_guia_id)) {
                    strncpy(msg, "Error al insertar cabecera de Guía", sizeof(msg)-1);
                } else {
                    // Insertar detalles con num_serie=new_guia_id, num_correlativo=i+1
                    bool todos_ok = true;
                    for (int i = 0; i < detalles_count; i++) {
                        int cant = atoi(detalles[i].cantidad_str);
                        double val = strtod(detalles[i].valor_unitario_str, NULL);
                        int num_serie = new_guia_id;
                        int num_correlativo = i + 1;
                        if (!db_add_detalle_guia(
                                id_emisor,
                                new_guia_id,
                                num_serie,
                                num_correlativo,
                                detalles[i].producto,
                                cant,
                                val,
                                detalles[i].descripcion)) {
                            todos_ok = false;
                            break;
                        }
                    }
                    if (todos_ok) {
                        log_auditoria(current_user, "Agregar guía y detalles", "Transacc_guia_remision/Transacc_detalle_guia");
                        strncpy(msg, "Guía y detalles registrados con éxito", sizeof(msg)-1);
                        // Limpiar campos de UI
                        fecha_emision[0] = fecha_inicio[0] = punto_partida[0] = punto_llegada[0] = motivo[0] = estado[0] = descripcion[0] = '\0';
                        // Limpiar lista de detalles
                        free(detalles);
                        detalles = NULL;
                        detalles_count = detalles_cap = 0;
                    } else {
                        strncpy(msg, "Error al insertar algún detalle", sizeof(msg)-1);
                        // Opcional: si deseas, eliminas la cabecera con DELETE WHERE id_guia=new_guia_id
                    }
                }
            }
        }
        msg[sizeof(msg)-1] = '\0';
    }

    // Botón Volver
    if (GuiButton((Rectangle){20, yBase + 10*verticalSpacingInputs, 80, 30}, "Volver")) {
        // Liberar memoria de listas
        if (clientes) {
            for (int i = 0; i < clientes_count; i++) free(clientes[i]);
            free(clientes); free(clientes_ids);
            clientes = NULL; clientes_ids = NULL; clientes_count = 0;
        }
        free(clientes_items_str); clientes_items_str = NULL;

        if (transportistas) {
            for (int i = 0; i < transportistas_count; i++) free(transportistas[i]);
            free(transportistas); free(transportistas_ids);
            transportistas = NULL; transportistas_ids = NULL; transportistas_count = 0;
        }
        free(transportistas_items_str); transportistas_items_str = NULL;

        if (vehiculos) {
            for (int i = 0; i < vehiculos_count; i++) free(vehiculos[i]);
            free(vehiculos); free(vehiculos_ids);
            vehiculos = NULL; vehiculos_ids = NULL; vehiculos_count = 0;
        }
        free(vehiculos_items_str); vehiculos_items_str = NULL;

        if (tipos_envio) {
            for (int i = 0; i < tipos_envio_count; i++) free(tipos_envio[i]);
            free(tipos_envio); free(tipos_envio_ids);
            tipos_envio = NULL; tipos_envio_ids = NULL; tipos_envio_count = 0;
        }
        free(tipos_envio_items_str); tipos_envio_items_str = NULL;

        loaded = false;

        // Reset campos cabecera
        fecha_emision[0] = fecha_inicio[0] = punto_partida[0] = punto_llegada[0] = motivo[0] = estado[0] = descripcion[0] = '\0';
        editFechaEmi = editFechaIni = editPartida = editLlegada = editMotivo = editEstado = editDesc = false;
        editCliente = editTransportista = editVehiculo = editTipoEnvio = false;

        // Limpiar detalles
        if (detalles) free(detalles);
        detalles = NULL; detalles_count = detalles_cap = 0;
        producto_tmp[0] = cantidad_tmp[0] = valor_tmp[0] = descripcion_tmp[0] = '\0';

        msg[0] = '\0';
        SetScreen(2); // Ajusta según tu pantalla de origen
    }

    // Mostrar mensaje si existe
    if (msg[0] != '\0') {
        DrawText(msg, 20, yBase + 10*verticalSpacingInputs + 40, 14, RED);
    }
}


void ShowScreen_QueryGuias() {
    static char fecha_ini[12] = "";
    static char fecha_fin[12] = "";
    static char msg[256] = "";

    static bool editFechaIni = false;
    static bool editFechaFin = false;

    typedef struct {
        int id_guia;
        char fecha_emision[12];
        char cliente[128];
        char transportista[128];
        char vehiculo[64];
        char tipo_envio[32];
        char punto_partida[128];
        char punto_llegada[128];
        char estado[32];
        char motivo[128];
    } GuiaInfo;

    static GuiaInfo *resultados = NULL;
    static int resultados_count = 0;

    static int selected_result_index = -1;
    static char *items_str = NULL;
    static bool editCombo = false;

    DrawText("Consultar Guías de Remisión", 20, 20, 20, BLACK);

    // Fechas
    GuiLabel((Rectangle){20, 60, 180, 20}, "Fecha inicio (YYYY-MM-DD):");
    if (GuiTextBox((Rectangle){220, 60, 120, 20}, fecha_ini, 11, editFechaIni)) {
        editFechaIni = !editFechaIni;
    }
    GuiLabel((Rectangle){20, 100, 180, 20}, "Fecha fin (YYYY-MM-DD):");
    if (GuiTextBox((Rectangle){220, 100, 120, 20}, fecha_fin, 11, editFechaFin)) {
        editFechaFin = !editFechaFin;
    }

    // Botón Aceptar / Buscar
    if (GuiButton((Rectangle){220, 140, 100, 30}, "Aceptar")) {
        if (!validar_fecha_yyyy_mm_dd(fecha_ini) || !validar_fecha_yyyy_mm_dd(fecha_fin)) {
            strncpy(msg, "Formato de fecha inválido (YYYY-MM-DD)", sizeof(msg)-1);
            msg[sizeof(msg)-1] = '\0';
        } else if (comparar_fechas_yyyy_mm_dd(fecha_ini, fecha_fin) > 0) {
            strncpy(msg, "Fecha inicio debe ser ≤ Fecha fin", sizeof(msg)-1);
            msg[sizeof(msg)-1] = '\0';
        } else {
            // Liberar previos
            if (resultados) {
                free(resultados);
                resultados = NULL;
                resultados_count = 0;
            }
            if (items_str) {
                free(items_str);
                items_str = NULL;
            }
            selected_result_index = -1;

            // Preparar consulta con joins para info enriquecida
            const char *sql =
                "SELECT gr.id_guia, gr.fecha_emision, "
                "       c.razon_social AS cliente, "
                "       t.nombre AS transportista, "
                "       (v.marca || ' ' || v.modelo) AS vehiculo, "
                "       te.tipo AS tipo_envio, "
                "       gr.punto_partida, gr.punto_llegada, gr.estado, gr.motivo_traslado "
                "FROM Transacc_guia_remision gr "
                "LEFT JOIN Mae_cliente c ON gr.id_cliente = c.id_cliente "
                "LEFT JOIN Mae_transportista t ON gr.id_transportista = t.id_transportista "
                "LEFT JOIN Mae_vehiculo v ON gr.id_vehiculo = v.id_vehiculo "
                "LEFT JOIN Mae_tipo_envio te ON gr.id_envio = te.id_envio "
                "WHERE gr.fecha_emision >= ? AND gr.fecha_emision <= ? "
                "ORDER BY gr.fecha_emision ASC;";
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
                strncpy(msg, "Error preparando consulta SQL", sizeof(msg)-1);
                msg[sizeof(msg)-1] = '\0';
            } else {
                sqlite3_bind_text(stmt, 1, fecha_ini, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, fecha_fin, -1, SQLITE_STATIC);
                int cap = 16;
                resultados = malloc(sizeof(GuiaInfo) * cap);
                if (!resultados) {
                    strncpy(msg, "Error de memoria al asignar resultados", sizeof(msg)-1);
                    msg[sizeof(msg)-1] = '\0';
                } else {
                    resultados_count = 0;
                    while (sqlite3_step(stmt) == SQLITE_ROW) {
                        if (resultados_count >= cap) {
                            cap *= 2;
                            GuiaInfo *tmp = realloc(resultados, sizeof(GuiaInfo) * cap);
                            if (!tmp) break;
                            resultados = tmp;
                        }
                        GuiaInfo *g = &resultados[resultados_count];
                        g->id_guia = sqlite3_column_int(stmt, 0);
                        const unsigned char *fe = sqlite3_column_text(stmt, 1);
                        const char *fe_text = fe ? (const char*)fe : "";
                        strncpy(g->fecha_emision, fe_text, sizeof(g->fecha_emision)-1);
                        g->fecha_emision[sizeof(g->fecha_emision)-1] = '\0';
                        // cliente
                        const unsigned char *cli = sqlite3_column_text(stmt, 2);
                        const char *cli_text = cli ? (const char*)cli : "";
                        strncpy(g->cliente, cli_text, sizeof(g->cliente)-1);
                        g->cliente[sizeof(g->cliente)-1] = '\0';
                        // transportista
                        const unsigned char *tra = sqlite3_column_text(stmt, 3);
                        const char *tra_text = tra ? (const char*)tra : "";
                        strncpy(g->transportista, tra_text, sizeof(g->transportista)-1);
                        g->transportista[sizeof(g->transportista)-1] = '\0';
                        // vehiculo
                        const unsigned char *veh = sqlite3_column_text(stmt, 4);
                        const char *veh_text = veh ? (const char*)veh : "";
                        strncpy(g->vehiculo, veh_text, sizeof(g->vehiculo)-1);
                        g->vehiculo[sizeof(g->vehiculo)-1] = '\0';
                        // tipo_envio
                        const unsigned char *tipe = sqlite3_column_text(stmt, 5);
                        const char *tipe_text = tipe ? (const char*)tipe : "";
                        strncpy(g->tipo_envio, tipe_text, sizeof(g->tipo_envio)-1);
                        g->tipo_envio[sizeof(g->tipo_envio)-1] = '\0';
                        // punto_partida
                        const unsigned char *pp = sqlite3_column_text(stmt, 6);
                        const char *pp_text = pp ? (const char*)pp : "";
                        strncpy(g->punto_partida, pp_text, sizeof(g->punto_partida)-1);
                        g->punto_partida[sizeof(g->punto_partida)-1] = '\0';
                        // punto_llegada
                        const unsigned char *pl = sqlite3_column_text(stmt, 7);
                        const char *pl_text = pl ? (const char*)pl : "";
                        strncpy(g->punto_llegada, pl_text, sizeof(g->punto_llegada)-1);
                        g->punto_llegada[sizeof(g->punto_llegada)-1] = '\0';
                        // estado
                        const unsigned char *est = sqlite3_column_text(stmt, 8);
                        const char *est_text = est ? (const char*)est : "";
                        strncpy(g->estado, est_text, sizeof(g->estado)-1);
                        g->estado[sizeof(g->estado)-1] = '\0';
                        // motivo
                        const unsigned char *mot = sqlite3_column_text(stmt, 9);
                        const char *mot_text = mot ? (const char*)mot : "";
                        strncpy(g->motivo, mot_text, sizeof(g->motivo)-1);
                        g->motivo[sizeof(g->motivo)-1] = '\0';

                        resultados_count++;
                    }
                    sqlite3_finalize(stmt);
                    if (resultados_count == 0) {
                        strncpy(msg, "No se encontraron guías en ese rango", sizeof(msg)-1);
                        msg[sizeof(msg)-1] = '\0';
                    } else {
                        snprintf(msg, sizeof(msg)-1, "Se encontraron %d guías", resultados_count);
                        msg[sizeof(msg)-1] = '\0';

                        // Construir items_str para ComboBox
                        int total_len = 0;
                        for (int i = 0; i < resultados_count; i++) {
                            total_len += 50 + strlen(resultados[i].cliente) + strlen(resultados[i].fecha_emision) + 5;
                        }
                        total_len += resultados_count + 1;
                        items_str = malloc(total_len);
                        if (items_str) {
                            items_str[0] = '\0';
                            for (int i = 0; i < resultados_count; i++) {
                                char line[256];
                                char cli_short[32] = "";
                                strncpy(cli_short, resultados[i].cliente, 31); cli_short[31] = '\0';
                                snprintf(line, sizeof(line), "ID:%d | Fecha:%s | Cliente:%.31s",
                                         resultados[i].id_guia,
                                         resultados[i].fecha_emision,
                                         cli_short);
                                strcat(items_str, line);
                                if (i < resultados_count - 1) strcat(items_str, "\n");
                            }
                            selected_result_index = 0;
                        }
                    }
                }
            }
        }
    }

    // Botón Descargar CSV (igual que antes)...
    if (GuiButton((Rectangle){20, 500, 150, 30}, "Descargar CSV")) {
        if (resultados_count <= 0) {
            strncpy(msg, "Primero ejecute la consulta", sizeof(msg)-1);
            msg[sizeof(msg)-1] = '\0';
        } else {
            // Lógica de CSV...
            log_auditoria(current_user, "Exportar CSV listado guías", "Transacc_guia_remision");
            // ...
        }
    }

    // Selección de guía particular en ComboBox
    int comboY = 220;
    if (resultados_count > 0 && items_str) {
        GuiLabel((Rectangle){20, comboY, 150, 20}, "Seleccione Guía:");
        if (GuiComboBox((Rectangle){180, comboY, 300, 20}, items_str, &selected_result_index)) {
            snprintf(msg, sizeof(msg)-1, "Seleccionada guía ID %d", resultados[selected_result_index].id_guia);
            msg[sizeof(msg)-1] = '\0';
        }
        comboY += 30;

        // Botón para generar PDF individual
        if (GuiButton((Rectangle){20, comboY, 200, 30}, "Generar PDF de esta guía")) {
            int id_sel = resultados[selected_result_index].id_guia;
            if (generate_pdf_guia(id_sel)) {
                snprintf(msg, sizeof(msg)-1, "PDF de guía %d generado", id_sel);
                msg[sizeof(msg)-1] = '\0';
                log_auditoria(current_user, "Exportar PDF guía individual", "Transacc_guia_remision/Transacc_detalle_guia");
            } else {
                snprintf(msg, sizeof(msg)-1, "Error al generar PDF guía %d", id_sel);
                msg[sizeof(msg)-1] = '\0';
            }
        }
        comboY += 40;
    }

    // Botón Volver
    if (GuiButton((Rectangle){20, 550, 100, 30}, "Volver")) {
        fecha_ini[0] = fecha_fin[0] = '\0';
        editFechaIni = editFechaFin = false;
        if (resultados) {
            free(resultados);
            resultados = NULL;
            resultados_count = 0;
        }
        if (items_str) {
            free(items_str);
            items_str = NULL;
        }
        selected_result_index = -1;
        msg[0] = '\0';
        SetScreen(2);
    }

    // Mensaje
    if (msg[0] != '\0') {
        //DrawText(msg, 20, comboY + 40, 14, RED);
	DrawText(msg, 140, 550, 14, RED);
    }

    // Mostrar resumen en pantalla (opcional)
    if (resultados_count > 0) {
        int y = 290;
        int lineHeight = 20;
        int maxY = GetScreenHeight() - 20;
        DrawText("Resumen (ID | Fecha | Cliente | Transporte | Estado):", 20, y, 12, DARKGRAY);
        y += lineHeight;
        for (int i = 0; i < resultados_count; i++) {
            GuiaInfo *g = &resultados[i];
            char line[512];
            char cli_short[24] = "";
            char tra_short[24] = "";
            strncpy(cli_short, g->cliente, 23); cli_short[23] = '\0';
            strncpy(tra_short, g->transportista, 23); tra_short[23] = '\0';
            snprintf(line, sizeof(line), "%d | %s | %.23s | %.23s | %s",
                     g->id_guia, g->fecha_emision, cli_short, tra_short, g->estado);
            DrawText(line, 20, y, 12, BLACK);
            y += lineHeight;
            if (y > maxY) break;
        }
    }
}



void ShowScreen_Warning() {
    DrawText("Acceso Denegado", 20, 20, 20, RED);
    DrawText("No tiene permisos suficientes", 20, 60, 20, BLACK);
    if (GuiButton((Rectangle){20, 100, 100, 30}, "Aceptar")) {
        SetScreen(2);
    }
}
