#ifndef SCREENS_H
#define SCREENS_H

// Inicialización y liberación de recursos de pantallas (si aplica)
void InitScreens(void);
void UnloadScreens(void);

// Navegación de pantallas
void SetScreen(int s);
int GetScreen(void);
// Dibuja la pantalla actual según el valor interno
void DrawCurrentScreen(void);

// Funciones de cada pantalla
void ShowScreen_Login(void);
void ShowScreen_Register(void);
void ShowScreen_MainMenu(void);
void ShowScreen_AddCliente(void);
void ShowScreen_AddTransportista(void);
void ShowScreen_UpdateEmpresa(void);
void ShowScreen_GuiaRemision(void);
void ShowScreen_QueryGuias(void);
void ShowScreen_Warning(void);

#endif // SCREENS_H
