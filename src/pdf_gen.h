// pdf_gen.h
#ifndef PDF_GEN_H
#define PDF_GEN_H

#include <stdbool.h>

// Genera el PDF de la guía con id_guia. Retorna true si se creó correctamente (archivo escrito).
bool generate_pdf_guia(int id_guia);

// (Opcional) Si implementas función para listado PDF completo:
// bool generate_pdf_query(const char *fecha_ini, const char *fecha_fin, const GuiaInfo *resultados, int resultados_count);

#endif // PDF_GEN_H
