#include "utils.h"
#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>

void log_auditoria(int user_id, const char *action, const char *entity) {
    char ts[64];
    time_t now = time(NULL);
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO Mae_auditoria(id_usuario, accion, fecha_hora, entidad_afectada) VALUES(?,?,?,?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return;
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, action, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, ts, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, entity, -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void start_session(int user_id) {
    char ts[64];
    time_t now = time(NULL);
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO Mae_sesion(id_usuario, fecha_inicio, ip) VALUES(?,?,?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return;
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, ts, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, "127.0.0.1", -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void end_session(int session_id) {
    char ts[64];
    time_t now = time(NULL);
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE Mae_sesion SET fecha_fin=? WHERE id_sesion=?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return;
    sqlite3_bind_text(stmt, 1, ts, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, session_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

bool validar_texto(const char *text, int max_len) {
    if (!text) return false;
    int len = strlen(text);
    if (len == 0 || len > max_len) return false;
    return true;
}

bool validar_numero(const char *text) {
    if (!text) return false;
    int len = strlen(text);
    if (len == 0) return false;
    for (int i = 0; i < len; i++) {
        if (!isdigit((unsigned char)text[i])) return false;
    }
    return true;
}

// Sólo letras (mayúsculas/minúsculas) y espacios. No vacíos. Long <= max_len.
bool validar_nombre(const char *text, int max_len) {
    if (!text) return false;
    int len = strlen(text);
    if (len == 0 || len > max_len) return false;
    for (int i = 0; i < len; i++) {
        char c = text[i];
        if (!(isalpha((unsigned char)c) || c == ' ')) return false;
    }
    return true;
}

// Email básico: no espacios, contiene una '@' no al inicio ni final, y al menos un '.' después de '@'
bool validar_email(const char *email, int max_len) {
    if (!email) return false;
    int len = strlen(email);
    if (len == 0 || len > max_len) return false;
    const char *at = strchr(email, '@');
    if (!at || at == email) return false;
    const char *dot = strchr(at + 1, '.');
    if (!dot || dot == at + 1) return false;
    if (dot == email + len - 1) return false; // no termina en '.'
    // No espacios
    for (int i = 0; i < len; i++) {
        if (isspace((unsigned char)email[i])) return false;
    }
    return true;
}

// RUC Perú: 11 dígitos
bool validar_ruc(const char *ruc) {
    if (!ruc) return false;
    int len = strlen(ruc);
    if (len != 11) return false;
    for (int i = 0; i < len; i++) {
        if (!isdigit((unsigned char)ruc[i])) return false;
    }
    return true;
}

// DNI Perú: 8 dígitos
bool validar_dni(const char *dni) {
    if (!dni) return false;
    int len = strlen(dni);
    if (len != 8) return false;
    for (int i = 0; i < len; i++) {
        if (!isdigit((unsigned char)dni[i])) return false;
    }
    return true;
}

// Teléfono: sólo dígitos, longitud razonable 7..15
bool validar_telefono(const char *tel) {
    if (!tel) return false;
    int len = strlen(tel);
    if (len < 7 || len > 15) return false;
    for (int i = 0; i < len; i++) {
        if (!isdigit((unsigned char)tel[i])) return false;
    }
    return true;
}

// URL básico: no espacios, contiene al menos un '.', y longitud <= max_len.
// Opcionalmente empieza con "http://" o "https://" o "www." o directamente dominio.tld
bool validar_url(const char *url, int max_len) {
    if (!url) return false;
    int len = strlen(url);
    if (len == 0 || len > max_len) return false;
    // No espacios
    for (int i = 0; i < len; i++) {
        if (isspace((unsigned char)url[i])) return false;
    }
    // Contiene al menos un '.'
    if (!strchr(url, '.')) return false;
    // if (!(strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0 || strncmp(url, "www.", 4) == 0)) return false;
    return true;
}

// Año: 4 dígitos numéricos, entre 1900 y (año actual+1)
bool validar_anio(const char *anio_str) {
    if (!anio_str) return false;
    int len = strlen(anio_str);
    if (len != 4) return false;
    for (int i = 0; i < 4; i++) {
        if (!isdigit((unsigned char)anio_str[i])) return false;
    }
    int anio = atoi(anio_str);
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    int current_year = tm.tm_year + 1900;
    if (anio < 1900 || anio > current_year + 1) return false;
    return true;
}


bool validar_fecha_yyyy_mm_dd(const char *fecha) {
    // Formato simple: longitud 10, posiciones 4 y 7 son '-', y los demás dígitos
    if (!fecha) return false;
    if (strlen(fecha) != 10) return false;
    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) {
            if (fecha[i] != '-') return false;
        } else {
            if (!isdigit((unsigned char)fecha[i])) return false;
        }
    }
    // Podrías validar valores de mes y día, pero aquí asumimos formato correcto
    return true;
}


bool validar_licencia(const char *s) {
    if (!s) return false;
    int len = strlen(s);
    if (len != 9) return false; // 1 letra + 8 dígitos = 9 caracteres
    // Primera posición letra
    unsigned char c0 = (unsigned char)s[0];
    if (!((c0 >= 'A' && c0 <= 'Z') || (c0 >= 'a' && c0 <= 'z'))) return false;
    // Siguientes 8 caracteres: dígitos
    for (int i = 1; i < 9; i++) {
        if (!isdigit((unsigned char)s[i])) return false;
    }
    return true;
}

bool validar_placa(const char *s) {
    if (!s) return false;
    int len = strlen(s);
    if (len != 6) return false;
    for (int i = 0; i < 6; i++) {
        unsigned char c = (unsigned char)s[i];
        if (!( (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') )) {
            return false;
        }
    }
    return true;
}

// Asume formato "YYYY-MM-DD", 10 caracteres, con dígitos y guiones en posiciones 4 y 7.
// Retorna INT_MIN si formato inválido; de lo contrario, diferencia lexicográfica de struct tm o equivalente.
static bool parse_yyyy_mm_dd(const char *s, int *year, int *month, int *day) {
    if (!s) return false;
    // longitud exacta 10
    if (strlen(s) != 10) return false;
    // posiciones 4 y 7 son '-'
    if (s[4] != '-' || s[7] != '-') return false;
    // extraer partes
    char buf[5];
    // año
    memcpy(buf, s, 4);
    buf[4] = '\0';
    *year = atoi(buf);
    // mes
    memcpy(buf, s+5, 2);
    buf[2] = '\0';
    *month = atoi(buf);
    // día
    memcpy(buf, s+8, 2);
    buf[2] = '\0';
    *day = atoi(buf);
    // Validar rangos básicos
    if (*year < 1900 || *year > 3000) return false;
    if (*month < 1 || *month > 12) return false;
    if (*day < 1 || *day > 31) return false;
    // Validar días según mes
    int mdays = 31;
    if (*month == 4 || *month == 6 || *month == 9 || *month == 11) mdays = 30;
    else if (*month == 2) {
        // Chequear año bisiesto
        bool bisiesto = ((*year % 4 == 0 && *year % 100 != 0) || (*year % 400 == 0));
        mdays = bisiesto ? 29 : 28;
    }
    if (*day > mdays) return false;
    return true;
}

int comparar_fechas_yyyy_mm_dd(const char *date1, const char *date2) {
    int y1,m1,d1;
    int y2,m2,d2;
    if (!parse_yyyy_mm_dd(date1, &y1, &m1, &d1)) return INT_MIN;
    if (!parse_yyyy_mm_dd(date2, &y2, &m2, &d2)) return INT_MIN;
    if (y1 != y2) return y1 - y2;
    if (m1 != m2) return m1 - m2;
    return d1 - d2;
}
