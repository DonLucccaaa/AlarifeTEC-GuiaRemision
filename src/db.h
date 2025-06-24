#ifndef DB_H
#define DB_H

#include <sqlite3.h>
#include <stdbool.h>
#include <time.h>

// Declaración del handle global de BD
extern sqlite3 *db;

// Apertura/cierre/inicialización
bool db_open(const char *filename);
void db_close();
bool db_init_schema();
bool db_exec(const char *sql);

// CRUD usuarios
bool db_verify_user(const char *username, const char *password, int *out_id, int *out_role);
bool db_create_user(const char *username, const char *email, const char *password, int role_id);

// Entidades básicas
bool db_add_cliente(const char *razon, const char *ruc, const char *direccion, const char *correo, const char *telefono);
bool db_update_empresa(int id, const char *razon, const char *ruc, const char *direccion, const char *web, const char *correo, const char *telefono, const char *lema);
bool db_get_empresa(int id, char *razon, char *ruc, char *direccion, char *web, char *correo, char *telefono, char *lema);
bool db_add_transportista(const char *nombre, const char *licencia, const char *telefono, int *out_transportista_id);
bool db_add_vehiculo(const char *placa, const char *marca, const char *modelo, int anio, int transportista_id);
bool db_add_detalle_guia(int id_empresa, int id_guia,
                         int num_serie, int num_correlativo,
                         const char *producto_servicio,
                         int cantidad,
                         double valor_unitario,
                         const char *descripcion);

// Carga genérica de lista de un solo campo (solo labels), para casos simples
// El caller deberá liberar cada (*out_labels)[i] y luego free(*out_labels).
bool db_load_list(const char *query, char ***out_labels, int *out_count);


bool db_load_list_with_ids(const char *query, int **out_ids, char ***out_labels, int *out_count);

// Función para insertar una guía (cabecera). Retorna id_guia en out_id_guia.
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
);

// Estructura para resultado consulta de guías
typedef struct {
    int id_guia;
    char fecha_emision[11+1];    
    char punto_partida[128];
    char punto_llegada[128];
    char estado[32];

} GuiaInfo;

// Consulta guías en un rango de fechas y devuelve resultados en arreglo dinámico.
// El caller debe liberar el arreglo con free(*out_resultados) tras uso.
bool db_query_guias_by_date(const char *fecha_inicio, const char *fecha_fin,
                            GuiaInfo **out_resultados, int *out_count);

// Funciones helper para UI: cargar listas específicas
// Estas funciones internamente llaman a db_load_list_with_ids con la consulta adecuada.
// El caller (pantalla) debe liberar memoria de labels y ids cuando ya no los necesite.
void cargar_clientes(char ***out_labels, int **out_ids, int *out_count);
void cargar_transportistas(char ***out_labels, int **out_ids, int *out_count);
void cargar_vehiculos(char ***out_labels, int **out_ids, int *out_count);
void cargar_tipos_envio(char ***out_labels, int **out_ids, int *out_count);

#endif 
