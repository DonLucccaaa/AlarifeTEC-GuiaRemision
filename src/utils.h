#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <time.h>

// Auditoría y sesión (ya existentes)
void log_auditoria(int user_id, const char *action, const char *entity);
void start_session(int user_id);
void end_session(int session_id);

// Validaciones sencillas actuales
bool validar_texto(const char *text, int max_len);  // no vacío, longitud <= max_len
bool validar_numero(const char *text);              // sólo dígitos, no vacío

// Nuevas validaciones:
bool validar_nombre(const char *text, int max_len); // sólo letras y espacios, no vacío, longitud <= max_len
bool validar_email(const char *email, int max_len); // contiene '@', al menos un '.', no espacios, longitud <= max_len
bool validar_ruc(const char *ruc);                  // sólo dígitos, longitud exacta 11
bool validar_dni(const char *dni);                  // sólo dígitos, longitud exacta 8
bool validar_telefono(const char *tel);             // sólo dígitos, longitud razonable (por ejemplo 7-15)
bool validar_url(const char *url, int max_len);     // formato básico: sin espacios, contiene al menos un '.', opcional prefijo http(s):// o www.
bool validar_anio(const char *anio_str);            // número, longitud 4, rango 1900..(año actual+1)
bool validar_fecha_yyyy_mm_dd(const char *fecha);   // Validar Fecha
bool validar_licencia(const char *s);               // Validar si contiene una letra y 8 digitos
bool validar_placa(const char *s);                  // Validar si contiene 6 caracteres
int comparar_fechas_yyyy_mm_dd(const char *date1, const char *date2); // Comparar fecha de emisión e inicio
#endif 
