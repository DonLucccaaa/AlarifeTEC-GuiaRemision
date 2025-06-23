// pdf_gen.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sqlite3.h>
#include <hpdf.h>     // Haru PDF
#include "db.h"       // para tus funciones de carga
#include "utils.h"    // validaciones, si necesario

// Estructuras para almacenar cabecera y detalle despues de consulta:
typedef struct {
    int id_guia;
    char fecha_emision[16];
    char fecha_inicio_traslado[16];
    char motivo_traslado[128];
    char punto_partida[128];
    char punto_llegada[128];
    char estado[32];
    char descripcion[256];
    int id_envio;
    int id_cliente;
    int id_emisor;
    int id_receptor;
    int id_transportista;
    int id_vehiculo;
    // campos de nombre
    char cliente_nombre[128];
    char transportista_nombre[128];
    char vehiculo_descripcion[64];
    char emisor_nombre[128];
    char emisor_ruc[32];
    char emisor_direccion[256];
    char emisor_telefono[32];
    char emisor_correo[64];
    char tipo_envio_nombre[32];
} CabeceraGuia;

typedef struct {
    int num_serie;
    int num_correlativo;
    char producto_servicio[128];
    int cantidad;
    double valor_unitario;
    char descripcion[256];
} DetalleGuia;

// Stub para cargar cabecera de guia. Implementar con SQLite.
// Retorna true si carga con exito.
bool cargar_cabecera_guia(int id_guia, CabeceraGuia *out) {
    sqlite3_stmt *stmt;
    const char *sql =
      "SELECT gr.fecha_emision, gr.fecha_inicio_traslado, gr.motivo_traslado, gr.punto_partida, gr.punto_llegada,"
      "       gr.estado, gr.descripcion, gr.id_envio, gr.id_cliente, gr.id_emisor, gr.id_receptor, gr.id_transportista, gr.id_vehiculo,"
      "       c.razon_social as cliente_nombre,"
      "       t.nombre as transportista_nombre,"
      "       (SELECT marca || ' ' || modelo FROM Mae_vehiculo WHERE id_vehiculo = gr.id_vehiculo) as vehiculo_desc,"
      "       e.nombre as emisor_nombre, e.ruc as emisor_ruc, e.direccion_fiscal as emisor_direccion, e.telefono as emisor_telefono, e.correo as emisor_correo,"
      "       te.tipo as tipo_envio_nombre "
      "  FROM Transacc_guia_remision gr "
      "  LEFT JOIN Mae_cliente c ON gr.id_cliente = c.id_cliente "
      "  LEFT JOIN Mae_transportista t ON gr.id_transportista = t.id_transportista "
      "  LEFT JOIN Mae_empresa e ON gr.id_emisor = e.id_empresa "
      "  LEFT JOIN Mae_tipo_envio te ON gr.id_envio = te.id_envio "
      " WHERE gr.id_guia = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, id_guia);
    bool ok = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        CabeceraGuia tmp;
        tmp.id_guia = id_guia;
        const unsigned char *t;
        // Extraer campos uno a uno
        t = sqlite3_column_text(stmt, 0);
        if (t) strncpy(tmp.fecha_emision, (const char*)t, sizeof(tmp.fecha_emision)-1);
        tmp.fecha_emision[sizeof(tmp.fecha_emision)-1] = '\0';
        t = sqlite3_column_text(stmt, 1);
        if (t) strncpy(tmp.fecha_inicio_traslado, (const char*)t, sizeof(tmp.fecha_inicio_traslado)-1);
        tmp.fecha_inicio_traslado[sizeof(tmp.fecha_inicio_traslado)-1] = '\0';
        t = sqlite3_column_text(stmt, 2);
        if (t) strncpy(tmp.motivo_traslado, (const char*)t, sizeof(tmp.motivo_traslado)-1);
        tmp.motivo_traslado[sizeof(tmp.motivo_traslado)-1] = '\0';
        t = sqlite3_column_text(stmt, 3);
        if (t) strncpy(tmp.punto_partida, (const char*)t, sizeof(tmp.punto_partida)-1);
        tmp.punto_partida[sizeof(tmp.punto_partida)-1] = '\0';
        t = sqlite3_column_text(stmt, 4);
        if (t) strncpy(tmp.punto_llegada, (const char*)t, sizeof(tmp.punto_llegada)-1);
        tmp.punto_llegada[sizeof(tmp.punto_llegada)-1] = '\0';
        t = sqlite3_column_text(stmt, 5);
        if (t) strncpy(tmp.estado, (const char*)t, sizeof(tmp.estado)-1);
        tmp.estado[sizeof(tmp.estado)-1] = '\0';
        t = sqlite3_column_text(stmt, 6);
        if (t) strncpy(tmp.descripcion, (const char*)t, sizeof(tmp.descripcion)-1);
        tmp.descripcion[sizeof(tmp.descripcion)-1] = '\0';
        tmp.id_envio         = sqlite3_column_int(stmt, 7);
        tmp.id_cliente       = sqlite3_column_int(stmt, 8);
        tmp.id_emisor        = sqlite3_column_int(stmt, 9);
        tmp.id_receptor      = sqlite3_column_int(stmt, 10);
        tmp.id_transportista = sqlite3_column_int(stmt, 11);
        tmp.id_vehiculo      = sqlite3_column_int(stmt, 12);
        // Campos de nombre
        t = sqlite3_column_text(stmt, 13);
        if (t) strncpy(tmp.cliente_nombre, (const char*)t, sizeof(tmp.cliente_nombre)-1);
        tmp.cliente_nombre[sizeof(tmp.cliente_nombre)-1] = '\0';
        t = sqlite3_column_text(stmt, 14);
        if (t) strncpy(tmp.transportista_nombre, (const char*)t, sizeof(tmp.transportista_nombre)-1);
        tmp.transportista_nombre[sizeof(tmp.transportista_nombre)-1] = '\0';
        t = sqlite3_column_text(stmt, 15);
        if (t) strncpy(tmp.vehiculo_descripcion, (const char*)t, sizeof(tmp.vehiculo_descripcion)-1);
        tmp.vehiculo_descripcion[sizeof(tmp.vehiculo_descripcion)-1] = '\0';
        t = sqlite3_column_text(stmt, 16);
        if (t) strncpy(tmp.emisor_nombre, (const char*)t, sizeof(tmp.emisor_nombre)-1);
        tmp.emisor_nombre[sizeof(tmp.emisor_nombre)-1] = '\0';
        t = sqlite3_column_text(stmt, 17);
        if (t) strncpy(tmp.emisor_ruc, (const char*)t, sizeof(tmp.emisor_ruc)-1);
        tmp.emisor_ruc[sizeof(tmp.emisor_ruc)-1] = '\0';
        t = sqlite3_column_text(stmt, 18);
        if (t) strncpy(tmp.emisor_direccion, (const char*)t, sizeof(tmp.emisor_direccion)-1);
        tmp.emisor_direccion[sizeof(tmp.emisor_direccion)-1] = '\0';
        t = sqlite3_column_text(stmt, 19);
        if (t) strncpy(tmp.emisor_telefono, (const char*)t, sizeof(tmp.emisor_telefono)-1);
        tmp.emisor_telefono[sizeof(tmp.emisor_telefono)-1] = '\0';
        t = sqlite3_column_text(stmt, 20);
        if (t) strncpy(tmp.emisor_correo, (const char*)t, sizeof(tmp.emisor_correo)-1);
        tmp.emisor_correo[sizeof(tmp.emisor_correo)-1] = '\0';
        t = sqlite3_column_text(stmt, 21);
        if (t) strncpy(tmp.tipo_envio_nombre, (const char*)t, sizeof(tmp.tipo_envio_nombre)-1);
        tmp.tipo_envio_nombre[sizeof(tmp.tipo_envio_nombre)-1] = '\0';

        *out = tmp;
        ok = true;
    }
    sqlite3_finalize(stmt);
    return ok;
}

// Stub para cargar detalles de guia. Implementar con SQLite.
// Retorna true si carga con exito.
// Asigna arreglo dinamico a *out_array y numero de elementos a *out_count.
// Llamar free() en el arreglo despues de usar.
bool cargar_detalles_guia(int id_guia, DetalleGuia **out_array, int *out_count) {
    sqlite3_stmt *stmt;
    const char *sql =
      "SELECT num_serie, num_correlativo, producto_servicio, cantidad, valor_unitario, descripcion "
      "FROM Transacc_detalle_guia "
      "WHERE id_guia = ? "
      "ORDER BY num_correlativo ASC;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, id_guia);

    int capacity = 4;
    DetalleGuia *arr = malloc(sizeof(DetalleGuia) * capacity);
    if (!arr) {
        sqlite3_finalize(stmt);
        return false;
    }
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (count >= capacity) {
            int nueva = capacity * 2;
            DetalleGuia *tmp2 = realloc(arr, sizeof(DetalleGuia) * nueva);
            if (!tmp2) break;
            arr = tmp2;
            capacity = nueva;
        }
        DetalleGuia *d = &arr[count];
        d->num_serie = sqlite3_column_int(stmt, 0);
        d->num_correlativo = sqlite3_column_int(stmt, 1);
        const unsigned char *t;
        t = sqlite3_column_text(stmt, 2);
        if (t) strncpy(d->producto_servicio, (const char*)t, sizeof(d->producto_servicio)-1);
        d->producto_servicio[sizeof(d->producto_servicio)-1] = '\0';
        d->cantidad = sqlite3_column_int(stmt, 3);
        d->valor_unitario = sqlite3_column_double(stmt, 4);
        t = sqlite3_column_text(stmt, 5);
        if (t) strncpy(d->descripcion, (const char*)t, sizeof(d->descripcion)-1);
        d->descripcion[sizeof(d->descripcion)-1] = '\0';
        count++;
    }
    sqlite3_finalize(stmt);
    *out_array = arr;
    *out_count = count;
    return true;
}

// Handler de errores de Haru
void error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data) {
    fprintf(stderr, "HPDF error: code=%04X, detail=%u\n", (unsigned)error_no, (unsigned)detail_no);
    // Segun diseno, se puede abortar o longjmp
}

// Funcion para generar PDF de guia sin acentos en textos.
// Retorna true si se creo con exito.
bool generate_pdf_guia(int id_guia) {
    CabeceraGuia cab;
    if (!cargar_cabecera_guia(id_guia, &cab)) {
        fprintf(stderr, "Error al cargar cabecera de guia %d\n", id_guia);
        return false;
    }

    DetalleGuia *detalles = NULL;
    int detalles_count = 0;
    if (!cargar_detalles_guia(id_guia, &detalles, &detalles_count)) {
        fprintf(stderr, "Error al cargar detalles de guia %d\n", id_guia);
        return false;
    }

    // Inicializar Haru
    HPDF_Doc pdf = HPDF_New(error_handler, NULL);
    if (!pdf) {
        fprintf(stderr, "Error: no se pudo crear HPDF_Doc\n");
        free(detalles);
        return false;
    }
    HPDF_SetCompressionMode(pdf, HPDF_COMP_ALL);
    HPDF_SetInfoAttr(pdf, HPDF_INFO_TITLE, "GUIA DE REMISION");
    HPDF_SetInfoAttr(pdf, HPDF_INFO_AUTHOR, "MiSistema");
    HPDF_SetInfoAttr(pdf, HPDF_INFO_SUBJECT, "GUIA DE REMISION");

    // Crear pagina
    HPDF_Page page = HPDF_AddPage(pdf);
    HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);

    // Obtener fuente base (sin soporte directo para tildes, pero se evitan acentos)
    HPDF_Font font_bold = HPDF_GetFont(pdf, "Helvetica-Bold", NULL);
    HPDF_Font font_reg  = HPDF_GetFont(pdf, "Helvetica", NULL);

    // Coordenadas de referencia: origen en esquina inferior izquierda.
    const float page_width = HPDF_Page_GetWidth(page);
    const float page_height = HPDF_Page_GetHeight(page);
    float margin = 40;
    float y = page_height - margin;

    // 1) Titulo centrado
    {
        const char *titulo = "GUIA DE REMISION";
        HPDF_Page_BeginText(page);
        HPDF_Page_SetFontAndSize(page, font_bold, 16);
        // Calculo de ancho aproximado (funciona para ASCII)
        float text_width = HPDF_Page_TextWidth(page, titulo);
        float x = (page_width - text_width) / 2;
        HPDF_Page_TextOut(page, x, y, titulo);
        HPDF_Page_EndText(page);
    }
    y -= 30;

    // 2) Datos del emisor
    {
        char buf[512];
        HPDF_Page_BeginText(page);
        HPDF_Page_SetFontAndSize(page, font_reg, 10);
        snprintf(buf, sizeof(buf), "Empresa Emisora: %s", cab.emisor_nombre);
        HPDF_Page_TextOut(page, margin, y, buf);
        y -= 15;
        snprintf(buf, sizeof(buf), "RUC: %s", cab.emisor_ruc);
        HPDF_Page_TextOut(page, margin, y, buf);
        y -= 15;
        snprintf(buf, sizeof(buf), "Direccion: %s", cab.emisor_direccion);
        HPDF_Page_TextOut(page, margin, y, buf);
        y -= 15;
        snprintf(buf, sizeof(buf), "Telefono: %s  Correo: %s", cab.emisor_telefono, cab.emisor_correo);
        HPDF_Page_TextOut(page, margin, y, buf);
        HPDF_Page_EndText(page);
    }
    y -= 25;

    // 3) Numero de guia
    {
        char buf[128];
        HPDF_Page_BeginText(page);
        HPDF_Page_SetFontAndSize(page, font_bold, 12);
        snprintf(buf, sizeof(buf), "Numero de Guia: %d", cab.id_guia);
        HPDF_Page_TextOut(page, margin, y, buf);
        HPDF_Page_EndText(page);
    }
    y -= 20;

    // 4) Fechas, cliente, transportista, etc.
    {
        char buf[256];
        HPDF_Page_BeginText(page);
        HPDF_Page_SetFontAndSize(page, font_reg, 10);
        snprintf(buf, sizeof(buf), "Fecha Emision: %s    Fecha Inicio Traslado: %s",
                 cab.fecha_emision, cab.fecha_inicio_traslado);
        HPDF_Page_TextOut(page, margin, y, buf);
        HPDF_Page_EndText(page);
        y -= 20;

        HPDF_Page_BeginText(page);
        HPDF_Page_SetFontAndSize(page, font_reg, 10);
        snprintf(buf, sizeof(buf), "Cliente: %s", cab.cliente_nombre);
        HPDF_Page_TextOut(page, margin, y, buf);
        HPDF_Page_EndText(page);
        y -= 15;

        HPDF_Page_BeginText(page);
        HPDF_Page_SetFontAndSize(page, font_reg, 10);
        snprintf(buf, sizeof(buf), "Transportista: %s    Vehiculo: %s",
                 cab.transportista_nombre, cab.vehiculo_descripcion);
        HPDF_Page_TextOut(page, margin, y, buf);
        HPDF_Page_EndText(page);
        y -= 15;

        HPDF_Page_BeginText(page);
        HPDF_Page_SetFontAndSize(page, font_reg, 10);
        snprintf(buf, sizeof(buf), "Tipo Envio: %s", cab.tipo_envio_nombre);
        HPDF_Page_TextOut(page, margin, y, buf);
        HPDF_Page_EndText(page);
        y -= 20;

        HPDF_Page_BeginText(page);
        HPDF_Page_SetFontAndSize(page, font_reg, 10);
        snprintf(buf, sizeof(buf), "Punto Partida: %s", cab.punto_partida);
        HPDF_Page_TextOut(page, margin, y, buf);
        HPDF_Page_EndText(page);
        y -= 15;

        HPDF_Page_BeginText(page);
        HPDF_Page_SetFontAndSize(page, font_reg, 10);
        snprintf(buf, sizeof(buf), "Punto Llegada: %s", cab.punto_llegada);
        HPDF_Page_TextOut(page, margin, y, buf);
        HPDF_Page_EndText(page);
        y -= 20;

        HPDF_Page_BeginText(page);
        HPDF_Page_SetFontAndSize(page, font_reg, 10);
        snprintf(buf, sizeof(buf), "Motivo Traslado: %s", cab.motivo_traslado);
        HPDF_Page_TextOut(page, margin, y, buf);
        HPDF_Page_EndText(page);
        y -= 20;
    }

    // 5) Observaciones (texto largo sin acentos)
    if (strlen(cab.descripcion) > 0) {
        HPDF_Page_BeginText(page);
        HPDF_Page_SetFontAndSize(page, font_reg, 10);
        HPDF_Page_TextOut(page, margin, y, "Observaciones:");
        HPDF_Page_EndText(page);
        y -= 15;
        // Wrap simple: trozos de hasta ~80 caracteres
        const char *p = cab.descripcion;
        while (*p && y > margin) {
            char line[128];
            int len = 0;
            // copiar hasta espacio o fin
            while (*p && len < 80 && *p != '\n') {
                line[len++] = *p++;
            }
            line[len] = '\0';
            if (*p == '\n') p++;
            HPDF_Page_BeginText(page);
            HPDF_Page_SetFontAndSize(page, font_reg, 10);
            HPDF_Page_TextOut(page, margin + 15, y, line);
            HPDF_Page_EndText(page);
            y -= 15;
        }
        y -= 5;
    }

    // 6) Tabla de detalles
    {
        float col_num_serie_w = 50;
        float col_num_corr_w  = 60;
        float col_prod_w      = 200;
        float col_cant_w      = 50;
        float col_valor_w     = 60;
        float col_total_w     = 60;
        float x = margin;
        float table_y = y;

        // Encabezado superior
        HPDF_Page_SetLineWidth(page, 0.5);
        HPDF_Page_MoveTo(page, x, table_y);
        HPDF_Page_LineTo(page, x + col_num_serie_w + col_num_corr_w + col_prod_w + col_cant_w + col_valor_w + col_total_w, table_y);
        HPDF_Page_Stroke(page);

        float y_row = table_y - 15;
        // Titulos
        HPDF_Page_BeginText(page);
        HPDF_Page_SetFontAndSize(page, font_bold, 10);
        HPDF_Page_TextOut(page, x + 2, y_row + 2, "Serie");
        HPDF_Page_TextOut(page, x + col_num_serie_w + 2, y_row + 2, "Corr.");
        HPDF_Page_TextOut(page, x + col_num_serie_w + col_num_corr_w + 2, y_row + 2, "Producto/Servicio");
        HPDF_Page_TextOut(page, x + col_num_serie_w + col_num_corr_w + col_prod_w + 2, y_row + 2, "Cant.");
        HPDF_Page_TextOut(page, x + col_num_serie_w + col_num_corr_w + col_prod_w + col_cant_w + 2, y_row + 2, "Valor U.");
        HPDF_Page_TextOut(page, x + col_num_serie_w + col_num_corr_w + col_prod_w + col_cant_w + col_valor_w + 2, y_row + 2, "Total");
        HPDF_Page_EndText(page);

        // Linea despues de encabezado
        HPDF_Page_MoveTo(page, x, y_row - 2);
        HPDF_Page_LineTo(page, x + col_num_serie_w + col_num_corr_w + col_prod_w + col_cant_w + col_valor_w + col_total_w, y_row - 2);
        HPDF_Page_Stroke(page);

        y_row -= 20;
        double total_general = 0.0;
        for (int i = 0; i < detalles_count && y_row > margin; i++) {
            DetalleGuia *d = &detalles[i];
            double total_linea = d->cantidad * d->valor_unitario;
            total_general += total_linea;
            char bufcol[128];

            // Serie
            snprintf(bufcol, sizeof(bufcol), "%d", d->num_serie);
            HPDF_Page_BeginText(page);
            HPDF_Page_SetFontAndSize(page, font_reg, 10);
            HPDF_Page_TextOut(page, x + 2, y_row, bufcol);
            HPDF_Page_EndText(page);

            // Correlativo
            snprintf(bufcol, sizeof(bufcol), "%d", d->num_correlativo);
            HPDF_Page_BeginText(page);
            HPDF_Page_SetFontAndSize(page, font_reg, 10);
            HPDF_Page_TextOut(page, x + col_num_serie_w + 2, y_row, bufcol);
            HPDF_Page_EndText(page);

            // Producto/Servicio (truncar si largo)
            char prodbuf[60];
            strncpy(prodbuf, d->producto_servicio, sizeof(prodbuf)-1);
            prodbuf[sizeof(prodbuf)-1] = '\0';
            HPDF_Page_BeginText(page);
            HPDF_Page_SetFontAndSize(page, font_reg, 10);
            HPDF_Page_TextOut(page, x + col_num_serie_w + col_num_corr_w + 2, y_row, prodbuf);
            HPDF_Page_EndText(page);

            // Cantidad
            snprintf(bufcol, sizeof(bufcol), "%d", d->cantidad);
            HPDF_Page_BeginText(page);
            HPDF_Page_SetFontAndSize(page, font_reg, 10);
            HPDF_Page_TextOut(page, x + col_num_serie_w + col_num_corr_w + col_prod_w + 2, y_row, bufcol);
            HPDF_Page_EndText(page);

            // Valor unitario
            snprintf(bufcol, sizeof(bufcol), "%.2f", d->valor_unitario);
            HPDF_Page_BeginText(page);
            HPDF_Page_SetFontAndSize(page, font_reg, 10);
            HPDF_Page_TextOut(page, x + col_num_serie_w + col_num_corr_w + col_prod_w + col_cant_w + 2, y_row, bufcol);
            HPDF_Page_EndText(page);

            // Total linea
            snprintf(bufcol, sizeof(bufcol), "%.2f", total_linea);
            HPDF_Page_BeginText(page);
            HPDF_Page_SetFontAndSize(page, font_reg, 10);
            HPDF_Page_TextOut(page, x + col_num_serie_w + col_num_corr_w + col_prod_w + col_cant_w + col_valor_w + 2, y_row, bufcol);
            HPDF_Page_EndText(page);

            // Linea horizontal debajo
            HPDF_Page_MoveTo(page, x, y_row - 2);
            HPDF_Page_LineTo(page, x + col_num_serie_w + col_num_corr_w + col_prod_w + col_cant_w + col_valor_w + col_total_w, y_row - 2);
            HPDF_Page_Stroke(page);

            y_row -= 20;
        }
        // Total general al final
        if (y_row - 20 > margin) {
            y_row -= 10;
            char buftot[64];
            snprintf(buftot, sizeof(buftot), "Total General: %.2f", total_general);
            HPDF_Page_BeginText(page);
            HPDF_Page_SetFontAndSize(page, font_bold, 10);
            HPDF_Page_TextOut(page, x + col_num_serie_w + col_num_corr_w + col_prod_w + col_cant_w + col_valor_w + 2, y_row, buftot);
            HPDF_Page_EndText(page);
        }
        y = y_row;
    }

    // Guardar PDF en archivo con nombre basado en id_guia
    char filename[128];
    snprintf(filename, sizeof(filename), "guia_%03d.pdf", id_guia);
    HPDF_SaveToFile(pdf, filename);

    HPDF_Free(pdf);
    free(detalles);
    return true;
}
