#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

// Definir el sqlite3* global
sqlite3 *db = NULL;

// ---------------- Apertura/Cierre/Inicialización ----------------

bool db_open(const char *filename) {
    if (sqlite3_open(filename, &db) != SQLITE_OK) {
        fprintf(stderr, "No se pudo abrir DB: %s\n", sqlite3_errmsg(db));
        return false;
    }
    // Activar foreign keys
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
    return true;
}

void db_close() {
    if (db) {
        sqlite3_close(db);
        db = NULL;
    }
}

bool db_exec(const char *sql) {
    char *err = NULL;
    if (!db) return false;
    if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err);
        sqlite3_free(err);
        return false;
    }
    return true;
}

bool db_init_schema() {

    const char *schema =
        "CREATE TABLE IF NOT EXISTS Mae_rol ("
            "id_rol INTEGER PRIMARY KEY AUTOINCREMENT, "
            "nombre TEXT UNIQUE NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS Mae_usuario ("
            "id_usuario INTEGER PRIMARY KEY AUTOINCREMENT, "
            "id_rol INTEGER NOT NULL, "
            "nombre TEXT UNIQUE NOT NULL, "
            "correo TEXT UNIQUE NOT NULL, "
            "contrasena TEXT NOT NULL, "
            "FOREIGN KEY(id_rol) REFERENCES Mae_rol(id_rol)"
        ");"
        "CREATE TABLE IF NOT EXISTS Mae_empresa ("
            "id_empresa INTEGER PRIMARY KEY AUTOINCREMENT, "
            "nombre TEXT, "
            "ruc TEXT, "
            "direccion_fiscal TEXT, "
            "sitio_web TEXT, "
            "correo TEXT, "
            "telefono TEXT, "
            "lema_empresa TEXT"
        ");"
        "CREATE TABLE IF NOT EXISTS Mae_cliente ("
            "id_cliente INTEGER PRIMARY KEY AUTOINCREMENT, "
            "razon_social TEXT, "
            "ruc TEXT, "
            "direccion TEXT, "
            "correo TEXT, "
            "telefono TEXT"
        ");"
        "CREATE TABLE IF NOT EXISTS Mae_transportista ("
            "id_transportista INTEGER PRIMARY KEY AUTOINCREMENT, "
            "nombre TEXT, "
            "licencia_conducir TEXT, "
            "telefono TEXT"
        ");"
        "CREATE TABLE IF NOT EXISTS Mae_vehiculo ("
            "id_vehiculo INTEGER PRIMARY KEY AUTOINCREMENT, "
            "placa TEXT, "
            "marca TEXT, "
            "modelo TEXT, "
            "anio INTEGER, "
            "id_transportista INTEGER, "
            "FOREIGN KEY(id_transportista) REFERENCES Mae_transportista(id_transportista)"
        ");"
        "CREATE TABLE IF NOT EXISTS Mae_tipo_envio ("
            "id_envio INTEGER PRIMARY KEY AUTOINCREMENT, "
            "tipo TEXT UNIQUE"
        ");"
        "CREATE TABLE IF NOT EXISTS Mae_sesion ("
            "id_sesion INTEGER PRIMARY KEY AUTOINCREMENT, "
            "id_usuario INTEGER, "
            "fecha_inicio TEXT, "
            "fecha_fin TEXT, "
            "ip TEXT, "
            "FOREIGN KEY(id_usuario) REFERENCES Mae_usuario(id_usuario)"
        ");"
        "CREATE TABLE IF NOT EXISTS Mae_auditoria ("
            "id_auditoria INTEGER PRIMARY KEY AUTOINCREMENT, "
            "id_usuario INTEGER, "
            "accion TEXT, "
            "fecha_hora TEXT, "
            "entidad_afectada TEXT, "
            "FOREIGN KEY(id_usuario) REFERENCES Mae_usuario(id_usuario)"
        ");"
        "CREATE TABLE IF NOT EXISTS Transacc_guia_remision ("
            "id_guia INTEGER PRIMARY KEY AUTOINCREMENT, "
            "fecha_emision TEXT, "
            "fecha_inicio_traslado TEXT, "
            "motivo_traslado TEXT, "
            "punto_partida TEXT, "
            "punto_llegada TEXT, "
            "estado TEXT, "
            "descripcion TEXT, "
            "id_envio INTEGER, "
            "id_cliente INTEGER, "
            "id_emisor INTEGER, "
            "id_receptor INTEGER, "
            "id_transportista INTEGER, "
            "id_vehiculo INTEGER, "
            "FOREIGN KEY(id_envio) REFERENCES Mae_tipo_envio(id_envio), "
            "FOREIGN KEY(id_cliente) REFERENCES Mae_cliente(id_cliente), "
            "FOREIGN KEY(id_emisor) REFERENCES Mae_empresa(id_empresa), "
            "FOREIGN KEY(id_receptor) REFERENCES Mae_cliente(id_cliente), "
            "FOREIGN KEY(id_transportista) REFERENCES Mae_transportista(id_transportista), "
            "FOREIGN KEY(id_vehiculo) REFERENCES Mae_vehiculo(id_vehiculo)"
        ");"
        "CREATE TABLE IF NOT EXISTS Transacc_detalle_guia ("
            "id_detalle INTEGER PRIMARY KEY AUTOINCREMENT, "
            "id_empresa INTEGER, "
            "id_guia INTEGER, "
            "num_serie INTEGER, "
            "num_correlativo INTEGER, "
            "producto_servicio TEXT, "
            "cantidad INTEGER, "
            "valor_unitario REAL, "
            "descripcion TEXT, "
            "FOREIGN KEY(id_empresa) REFERENCES Mae_empresa(id_empresa), "
            "FOREIGN KEY(id_guia) REFERENCES Transacc_guia_remision(id_guia)"
        ");";
    if (!db_exec(schema)) return false;

    // Pre-popular roles:
    // Insertar si no existen, en este orden: Admin, Encargado, Consultor
    // Usamos INSERT OR IGNORE, y asumimos que si ya existe, conserva el mismo id_rol.
    db_exec("INSERT OR IGNORE INTO Mae_rol(nombre) VALUES('Admin');");
    db_exec("INSERT OR IGNORE INTO Mae_rol(nombre) VALUES('Encargado');");
    db_exec("INSERT OR IGNORE INTO Mae_rol(nombre) VALUES('Consultor');");

    // Insertar usuario admin si no existe:
    // Necesitamos nombre único “admin”. También se exige campo correo en la tabla Mae_usuario:
    // Decidimos usar correo “admin@local” o similar. 
    // La contraseña en texto plano “admin” 
    // Usamos SELECT para saber si ya existe:
    {
        sqlite3_stmt *stmt;
        const char *checkSql = "SELECT id_usuario FROM Mae_usuario WHERE nombre='admin';";
        if (sqlite3_prepare_v2(db, checkSql, -1, &stmt, NULL) == SQLITE_OK) {
            if (sqlite3_step(stmt) != SQLITE_ROW) {
                // No existe admin: lo insertamos
                sqlite3_finalize(stmt);
                // Obtener id_rol de Admin
                sqlite3_stmt *stmt2;
                const char *rolSql = "SELECT id_rol FROM Mae_rol WHERE nombre='Admin';";
                int adminRole = 1;
                if (sqlite3_prepare_v2(db, rolSql, -1, &stmt2, NULL) == SQLITE_OK) {
                    if (sqlite3_step(stmt2) == SQLITE_ROW) {
                        adminRole = sqlite3_column_int(stmt2, 0);
                    }
                }
                sqlite3_finalize(stmt2);
                // Insertar admin con nombre 'admin', correo 'admin@local', contraseña 'admin'
                sqlite3_stmt *stmtIns;
                const char *insSql = "INSERT INTO Mae_usuario(id_rol,nombre,correo,contrasena) VALUES(?,?,?,?);";
                if (sqlite3_prepare_v2(db, insSql, -1, &stmtIns, NULL) == SQLITE_OK) {
                    sqlite3_bind_int(stmtIns, 1, adminRole);
                    sqlite3_bind_text(stmtIns, 2, "admin", -1, SQLITE_STATIC);
                    sqlite3_bind_text(stmtIns, 3, "admin@local", -1, SQLITE_STATIC);
                    sqlite3_bind_text(stmtIns, 4, "admin", -1, SQLITE_STATIC);
                    sqlite3_step(stmtIns);
                }
                sqlite3_finalize(stmtIns);
            } else {
                sqlite3_finalize(stmt);
            }
        }
    }

    // También pre-popular Mae_tipo_envio u otros datos si es necesario...
    return true;
}

// ---------------- CRUD básico de usuarios ----------------

bool db_verify_user(const char *username, const char *password, int *out_id, int *out_role) {
    if (!db) return false;
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT id_usuario, id_rol, contrasena FROM Mae_usuario WHERE nombre = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    bool ok = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int uid = sqlite3_column_int(stmt, 0);
        int rid = sqlite3_column_int(stmt, 1);
        const unsigned char *stored = sqlite3_column_text(stmt, 2);
        if (stored && strcmp((const char*)stored, password) == 0) {
            *out_id = uid;
            *out_role = rid;
            ok = true;
        }
    }
    sqlite3_finalize(stmt);
    return ok;
}

bool db_create_user(const char *username, const char *email, const char *password, int role_id) {
    if (!db) return false;
    sqlite3_stmt *stmt = NULL;
    const char *sql = "INSERT INTO Mae_usuario(id_rol, nombre, correo, contrasena) VALUES(?,?,?,?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, role_id);
    sqlite3_bind_text(stmt, 2, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, email, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, password, -1, SQLITE_STATIC);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

// ---------------- Entidades básicas ----------------

bool db_add_cliente(const char *razon, const char *ruc, const char *direccion, const char *correo, const char *telefono) {
    if (!db) return false;
    sqlite3_stmt *stmt = NULL;
    const char *sql = "INSERT INTO Mae_cliente(razon_social, ruc, direccion, correo, telefono) VALUES(?,?,?,?,?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, razon, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, ruc, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, direccion, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, correo, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, telefono, -1, SQLITE_STATIC);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool db_update_empresa(int id, const char *razon, const char *ruc, const char *direccion, const char *web, const char *correo, const char *telefono, const char *lema) {
    if (!db) return false;
    sqlite3_stmt *stmt = NULL;
    const char *sql = "UPDATE Mae_empresa SET nombre=?, ruc=?, direccion_fiscal=?, sitio_web=?, correo=?, telefono=?, lema_empresa=? WHERE id_empresa=?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, razon, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, ruc, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, direccion, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, web, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, correo, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, telefono, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, lema, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 8, id);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool db_get_empresa(int id, char *razon, char *ruc, char *direccion, char *web, char *correo, char *telefono, char *lema) {
    if (!db) return false;
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT nombre, ruc, direccion_fiscal, sitio_web, correo, telefono, lema_empresa FROM Mae_empresa WHERE id_empresa=?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, id);
    bool ok = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *t0 = sqlite3_column_text(stmt, 0);
        const unsigned char *t1 = sqlite3_column_text(stmt, 1);
        const unsigned char *t2 = sqlite3_column_text(stmt, 2);
        const unsigned char *t3 = sqlite3_column_text(stmt, 3);
        const unsigned char *t4 = sqlite3_column_text(stmt, 4);
        const unsigned char *t5 = sqlite3_column_text(stmt, 5);
        const unsigned char *t6 = sqlite3_column_text(stmt, 6);
        if (t0) strncpy(razon, (const char*)t0, 63), razon[63]='\0';
        if (t1) strncpy(ruc, (const char*)t1, 15), ruc[15]='\0';
        if (t2) strncpy(direccion, (const char*)t2, 127), direccion[127]='\0';
        if (t3) strncpy(web, (const char*)t3, 63), web[63]='\0';
        if (t4) strncpy(correo, (const char*)t4, 63), correo[63]='\0';
        if (t5) strncpy(telefono, (const char*)t5, 15), telefono[15]='\0';
        if (t6) strncpy(lema, (const char*)t6, 127), lema[127]='\0';
        ok = true;
    }
    sqlite3_finalize(stmt);
    return ok;
}

bool db_add_transportista(const char *nombre, const char *licencia, const char *telefono, int *out_transportista_id) {
    if (!db) return false;
    sqlite3_stmt *stmt = NULL;
    const char *sql = "INSERT INTO Mae_transportista(nombre, licencia_conducir, telefono) VALUES(?,?,?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, nombre, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, licencia, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, telefono, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false;
    }
    if (out_transportista_id) {
        *out_transportista_id = (int)sqlite3_last_insert_rowid(db);
    }
    sqlite3_finalize(stmt);
    return true;
}

bool db_add_vehiculo(const char *placa, const char *marca, const char *modelo, int anio, int transportista_id) {
    if (!db) return false;
    sqlite3_stmt *stmt = NULL;
    const char *sql = "INSERT INTO Mae_vehiculo(placa, marca, modelo, anio, id_transportista) VALUES(?,?,?,?,?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, placa, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, marca, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, modelo, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, anio);
    sqlite3_bind_int(stmt, 5, transportista_id);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

// ---------------- Carga listas ----------------

// Carga lista de un solo campo TEXT (columna 0). El caller libera labels.
bool db_load_list(const char *query, char ***out_labels, int *out_count) {
    if (!db || !query || !out_labels || !out_count) return false;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "db_load_list prepare error: %s\n", sqlite3_errmsg(db));
        return false;
    }
    int capacity = 10;
    char **labels = malloc(sizeof(char*) * capacity);
    if (!labels) {
        sqlite3_finalize(stmt);
        return false;
    }
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *text = sqlite3_column_text(stmt, 0);
        const char *txt = text ? (const char*)text : "";
        if (count >= capacity) {
            capacity *= 2;
            char **tmp = realloc(labels, sizeof(char*) * capacity);
            if (!tmp) break;
            labels = tmp;
        }
        labels[count] = strdup(txt);
        if (!labels[count]) break;
        count++;
    }
    sqlite3_finalize(stmt);
    *out_labels = labels;
    *out_count = count;
    return true;
}

// Carga lista con IDs y labels. Consulta debe retornar al menos dos columnas: ID (col 0), label (col 1).
bool db_load_list_with_ids(const char *query, int **out_ids, char ***out_labels, int *out_count) {
    if (!db || !query || !out_ids || !out_labels || !out_count) return false;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "db_load_list_with_ids prepare error: %s\n", sqlite3_errmsg(db));
        return false;
    }
    int capacity = 10;
    int *ids = malloc(sizeof(int) * capacity);
    char **labels = malloc(sizeof(char*) * capacity);
    if (!ids || !labels) {
        sqlite3_finalize(stmt);
        free(ids); free(labels);
        return false;
    }
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (count >= capacity) {
            capacity *= 2;
            int *tmp_ids = realloc(ids, sizeof(int) * capacity);
            char **tmp_labels = realloc(labels, sizeof(char*) * capacity);
            if (!tmp_ids || !tmp_labels) {
                // liberar
                for (int i = 0; i < count; i++) free(labels[i]);
                free(labels); free(ids);
                sqlite3_finalize(stmt);
                return false;
            }
            ids = tmp_ids;
            labels = tmp_labels;
        }
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char *text = sqlite3_column_text(stmt, 1);
        const char *txt = text ? (const char*)text : "";
        ids[count] = id;
        labels[count] = strdup(txt);
        if (!labels[count]) {
            // liberar
            for (int i = 0; i < count; i++) free(labels[i]);
            free(labels); free(ids);
            sqlite3_finalize(stmt);
            return false;
        }
        count++;
    }
    sqlite3_finalize(stmt);
    *out_ids = ids;
    *out_labels = labels;
    *out_count = count;
    return true;
}

// Funciones helper para UI: cargar listas específicas
void cargar_clientes(char ***out_labels, int **out_ids, int *out_count) {
    const char *sql = "SELECT id_cliente, razon_social FROM Mae_cliente ORDER BY razon_social;";
    if (!db_load_list_with_ids(sql, out_ids, out_labels, out_count)) {
        *out_labels = NULL;
        *out_ids = NULL;
        *out_count = 0;
    }
}

void cargar_transportistas(char ***out_labels, int **out_ids, int *out_count) {
    const char *sql = "SELECT id_transportista, nombre FROM Mae_transportista ORDER BY nombre;";
    if (!db_load_list_with_ids(sql, out_ids, out_labels, out_count)) {
        *out_labels = NULL;
        *out_ids = NULL;
        *out_count = 0;
    }
}

void cargar_vehiculos(char ***out_labels, int **out_ids, int *out_count) {
    const char *sql = "SELECT id_vehiculo, placa FROM Mae_vehiculo ORDER BY placa;";
    if (!db_load_list_with_ids(sql, out_ids, out_labels, out_count)) {
        *out_labels = NULL;
        *out_ids = NULL;
        *out_count = 0;
    }
}

void cargar_tipos_envio(char ***out_labels, int **out_ids, int *out_count) {
    const char *sql = "SELECT id_envio, tipo FROM Mae_tipo_envio ORDER BY tipo;";
    if (!db_load_list_with_ids(sql, out_ids, out_labels, out_count)) {
        *out_labels = NULL;
        *out_ids = NULL;
        *out_count = 0;
    }
}

// ---------------- Guía de remisión ----------------

bool db_add_guia(
    const char *fecha_emision,
    const char *fecha_inicio_traslado,
    const char *motivo_traslado,
    const char *punto_partida,
    const char *punto_llegada,
    const char *estado,
    const char *descripcion,
    int id_envio,
    int id_cliente,
    int id_emisor,
    int id_receptor,
    int id_transportista,
    int id_vehiculo,
    int *out_id_guia
) {
    if (!db) return false;
    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "INSERT INTO Transacc_guia_remision("
            "fecha_emision, fecha_inicio_traslado, motivo_traslado, punto_partida, punto_llegada, "
            "estado, descripcion, id_envio, id_cliente, id_emisor, id_receptor, id_transportista, id_vehiculo"
        ") VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, fecha_emision, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, fecha_inicio_traslado, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, motivo_traslado, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, punto_partida, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, punto_llegada, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, estado, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, descripcion, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 8, id_envio);
    sqlite3_bind_int(stmt, 9, id_cliente);
    sqlite3_bind_int(stmt, 10, id_emisor);
    sqlite3_bind_int(stmt, 11, id_receptor);
    sqlite3_bind_int(stmt, 12, id_transportista);
    sqlite3_bind_int(stmt, 13, id_vehiculo);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (ok && out_id_guia) {
        *out_id_guia = (int)sqlite3_last_insert_rowid(db);
    }
    sqlite3_finalize(stmt);
    return ok;
}

// Consulta guías por fecha, devolviendo arreglo dinámico de GuiaInfo.
bool db_query_guias_by_date(const char *fecha_inicio, const char *fecha_fin,
                            GuiaInfo **out_resultados, int *out_count) {
    if (!db || !fecha_inicio || !fecha_fin || !out_resultados || !out_count) return false;
    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "SELECT id_guia, fecha_emision, punto_partida, punto_llegada, estado "
        "FROM Transacc_guia_remision "
        "WHERE fecha_emision >= ? AND fecha_emision <= ? "
        "ORDER BY fecha_emision ASC;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Error preparando consulta guías: %s\n", sqlite3_errmsg(db));
        return false;
    }
    sqlite3_bind_text(stmt, 1, fecha_inicio, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, fecha_fin, -1, SQLITE_STATIC);

    int capacity = 10;
    int count = 0;
    GuiaInfo *arr = malloc(sizeof(GuiaInfo) * capacity);
    if (!arr) {
        sqlite3_finalize(stmt);
        return false;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (count >= capacity) {
            capacity *= 2;
            GuiaInfo *tmp = realloc(arr, sizeof(GuiaInfo) * capacity);
            if (!tmp) break;
            arr = tmp;
        }
        GuiaInfo *g = &arr[count];
        g->id_guia = sqlite3_column_int(stmt, 0);

        const unsigned char *femi = sqlite3_column_text(stmt, 1);
        const char *femi_text = femi ? (const char*)femi : "";
        strncpy(g->fecha_emision, femi_text, sizeof(g->fecha_emision)-1);
        g->fecha_emision[sizeof(g->fecha_emision)-1] = '\0';

        const unsigned char *pp = sqlite3_column_text(stmt, 2);
        const char *pp_text = pp ? (const char*)pp : "";
        strncpy(g->punto_partida, pp_text, sizeof(g->punto_partida)-1);
        g->punto_partida[sizeof(g->punto_partida)-1] = '\0';

        const unsigned char *pl = sqlite3_column_text(stmt, 3);
        const char *pl_text = pl ? (const char*)pl : "";
        strncpy(g->punto_llegada, pl_text, sizeof(g->punto_llegada)-1);
        g->punto_llegada[sizeof(g->punto_llegada)-1] = '\0';

        const unsigned char *est = sqlite3_column_text(stmt, 4);
        const char *est_text = est ? (const char*)est : "";
        strncpy(g->estado, est_text, sizeof(g->estado)-1);
        g->estado[sizeof(g->estado)-1] = '\0';

        count++;
    }
    sqlite3_finalize(stmt);
    *out_resultados = arr;
    *out_count = count;
    return true;
}


// Inserta un detalle de guía, con serie y correlativo ya calculados externamente.
bool db_add_detalle_guia(int id_empresa, int id_guia,
                         int num_serie, int num_correlativo,
                         const char *producto_servicio,
                         int cantidad,
                         double valor_unitario,
                         const char *descripcion) {
    sqlite3_stmt *stmt;
    const char *sql =
        "INSERT INTO Transacc_detalle_guia("
            "id_empresa, id_guia, num_serie, num_correlativo, producto_servicio, cantidad, valor_unitario, descripcion"
        ") VALUES(?,?,?,?,?,?,?,?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, id_empresa);
    sqlite3_bind_int(stmt, 2, id_guia);
    sqlite3_bind_int(stmt, 3, num_serie);
    sqlite3_bind_int(stmt, 4, num_correlativo);
    sqlite3_bind_text(stmt, 5, producto_servicio, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, cantidad);
    sqlite3_bind_double(stmt, 7, valor_unitario);
    sqlite3_bind_text(stmt, 8, descripcion, -1, SQLITE_STATIC);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}
