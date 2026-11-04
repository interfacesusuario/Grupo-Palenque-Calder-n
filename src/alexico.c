/********************************************************************************
 * 1. INCLUSIÓN DE LIBRERÍAS Y CONSTANTES GLOBALES DE CONFIGURACIÓN
 * Esta sección incluye las librerías estándar de C necesarias para manejo de 
 * I/O, cadenas, conversión de tipos y clasificación de caracteres.
 * Define las constantes límite del lenguaje Razio:
 * - Límite de buffer de cadenas.
 * - Longitud máxima de identificadores (D6: 20 caracteres).
 * - Límite superior para enteros con signo de 32 bits (D2).
 ********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>

/*28/10*/
#include "sintactico.tab.h"	/* Incluye los tokens generados por Bison */

#define MAX_BUFFER 256
#define MAX_ID_LEN 20               /* D6: Longitud máxima de identificador */
#define MAX_INT_32 2147483647LL     /* D2: Límite de 32 bits con signo (64-bit safe) */



/********************************************************************************
 * 2. TABLA DE TOKENS (CÓDIGOS NUMÉRICOS PARA INTERFAZ CON YACC/BISON)
 * Define las constantes simbólicas numéricas asociadas a cada Token del lenguaje.
 * Se utilizan códigos a partir de 257 para no colisionar con los caracteres ASCII.
 ********************************************************************************/
 
  
 /* CAMBIO PARA LA PARTE SINTACTICA: 28/10*/
 /********************************************************************************
 * 2. TABLA DE TOKENS
 * Las constantes simbólicas (PR_DEF, ID, etc.) son provistas directamente
 * por sintactico.tab.h para garantizar coincidencia exacta de códigos numéricos.
 ********************************************************************************/
 /* 
#define PR_DEF           257
#define PR_PRINCIPAL     258
#define PR_RACIONAL      259
#define PR_SI            260
#define PR_SINO          261
#define PR_MIENTRAS      262
#define PR_MOSTRAR       263
#define PR_RETORNAR      264
#define ID               265
#define LIT_RACIONAL     266
#define LIT_CADENA       267
#define OP_ASIG          268
#define OP_SUMA          269
#define OP_RESTA         270
#define OP_MULT          271
#define OP_DIV           272
#define COMP_MAYOR       273
#define COMP_MENOR       274
#define COMP_MAYOR_IGUAL 275
#define COMP_MENOR_IGUAL 276
#define COMP_IGUAL       277
#define COMP_DISTINTO    278
#define OP_AND           279
#define OP_OR            280
#define PAR_IZQ          281
#define PAR_DER          282
#define LLAVE_IZQ        283
#define LLAVE_DER        284
#define PUNTO_COMA       285
#define COMA             286
*/

/********************************************************************************
 * 3. ESTRUCTURA Y GESTIÓN DE LA TABLA DE SÍMBOLOS
 * Define la estructura de datos que almacena los identificadores (ID), constantes
 * racionales y literales de cadena reconocidos por el analizador.
 * Incluye la función insertar_ts() para evitar duplicados y registrar atributos.
 ********************************************************************************/
typedef struct {
    char nombre[MAX_BUFFER];
    char tipo[20];
    char valor[MAX_BUFFER];
    int longitud;
} RegTablaSimbolos;

RegTablaSimbolos tabla_simbolos[500];
int cant_simbolos = 0;

void insertar_ts(const char *nombre, const char *tipo, const char *valor, int longitud) {
    for (int i = 0; i < cant_simbolos; i++) {
        if (strcmp(tabla_simbolos[i].nombre, nombre) == 0) return;
    }
    if (cant_simbolos < 500) {
        strcpy(tabla_simbolos[cant_simbolos].nombre, nombre);
        strcpy(tabla_simbolos[cant_simbolos].tipo, tipo);
        strcpy(tabla_simbolos[cant_simbolos].valor, valor);
        tabla_simbolos[cant_simbolos].longitud = longitud;
        cant_simbolos++;
    }
}

/********************************************************************************
 * 4. VARIABLES GLOBALES DE ESTADO DEL ANALIZADOR LÉXICO
 * Mantienen el estado interno del scanner durante el procesamiento del archivo:
 * - Contador de líneas para reporte de errores.
 * - Puntero al archivo fuente.
 * - Buffer temporal de caracteres para construir los lexemas.
 * - Último token identificado y acumulador para control de desbordamiento (D2).
 ********************************************************************************/
int num_linea = 1;
int error_lexico = 0;
FILE *inputFile = NULL;
char buffer[MAX_BUFFER];
int buf_idx = 0;
char c_actual;
int ultimo_token_devuelto = 0;
long long val_num_actual = 0;


/********************************************************************************
 * 5. CLASIFICACIÓN DEL ALFABETO EN EVENTOS (COLUMNAS DE LA MATRIZ)
 * Mapea el conjunto de caracteres de entrada (S) a las columnas numéricas de
 * la matriz de transiciones del autómata finito determinista.
 ********************************************************************************/

/* 1. Mapeo del alfabeto a columnas (0 a 19):
 * Define los números de columna de las matrices M1 (nuevo estado) y M2 (acción semántica).
 * NUM_COLUMNAS fija automáticamente en 20 el ancho total de las matrices. */
typedef enum {
    COL_LETRA = 0, COL_DIGITO, COL_MAS, COL_MENOS, COL_POR, COL_BARRA,
    COL_IGUAL, COL_MENOR, COL_MAYOR, COL_ADMIRACION, COL_PAR_IZQ,
    COL_PAR_DER, COL_LLAVE_IZQ, COL_LLAVE_DER, COL_PUNTO_COMA,
    COL_DOS_PUNTOS, COL_COMA, COL_COMILLA, COL_BLANCO, COL_OTRO,
    NUM_COLUMNAS
} Evento;

/* 2. Puntero a función semántica:
 * Permite almacenar funciones sin retorno ni parámetros (void f(void)) en la matriz M2
 * para ejecutarlas dinámicamente según el estado y carácter leído. */
typedef void (*AccionSemantica)(void);

/* 3. Traductor de caracteres a columna:
 * Recibe el carácter leído del archivo (c) y devuelve su número de columna correspondiente
 * para consultar M1 y M2. Todo carácter fuera del alfabeto cae en COL_OTRO (19). */
Evento get_evento(int c) {
    if (isalpha(c)) return COL_LETRA;
    if (isdigit(c)) return COL_DIGITO;
    switch (c) {
        case '+': return COL_MAS;
        case '-': return COL_MENOS;
        case '*': return COL_POR;
        case '/': return COL_BARRA;
        case '=': return COL_IGUAL;
        case '<': return COL_MENOR;
        case '>': return COL_MAYOR;
        case '!': return COL_ADMIRACION;
        case '(': return COL_PAR_IZQ;
        case ')': return COL_PAR_DER;
        case '{': return COL_LLAVE_IZQ;
        case '}': return COL_LLAVE_DER;
        case ';': return COL_PUNTO_COMA;
        case ':': return COL_DOS_PUNTOS;
        case ',': return COL_COMA;
        case '"': return COL_COMILLA;
		case ' ': case '\t': case '\r': case '\n': case '\0': case EOF: return COL_BLANCO; /* Mapea caracteres de fin como blanco */
        default: return COL_OTRO;
    }
}



/********************************************************************************
 * 6. TRADUCCIÓN DE CÓDIGOS DE TOKEN A CADENAS TEXTUALES
 * Traduce el código numérico interno del token (ej: 257) a su equivalente en texto (ej: "PR_DEF")
 * para mostrarlo de forma legible en la tabla del reporte de consola.
 * Cada caso está definido al inicio del código con #define en la tabla de tokens.
 ********************************************************************************/
const char* obtener_nombre_token(int codigo) {
    switch (codigo) {
        case PR_DEF: return "PR_DEF";
        case PR_PRINCIPAL: return "PR_PRINCIPAL";
        case PR_RACIONAL: return "PR_RACIONAL";
        case PR_SI: return "PR_SI";
        case PR_SINO: return "PR_SINO";
        case PR_MIENTRAS: return "PR_MIENTRAS";
        case PR_MOSTRAR: return "PR_MOSTRAR";
        case PR_RETORNAR: return "PR_RETORNAR";
        case ID: return "ID";
        case LIT_RACIONAL: return "LIT_RACIONAL";
        case LIT_CADENA: return "LIT_CADENA";
        case OP_ASIG: return "OP_ASIG";
        case OP_SUMA: return "OP_SUMA";
        case OP_RESTA: return "OP_RESTA";
        case OP_MULT: return "OP_MULT";
        case OP_DIV: return "OP_DIV";
        case COMP_MAYOR: return "COMP_MAYOR";
        case COMP_MENOR: return "COMP_MENOR";
        case COMP_MAYOR_IGUAL: return "COMP_MAYOR_IGUAL";
        case COMP_MENOR_IGUAL: return "COMP_MENOR_IGUAL";
        case COMP_IGUAL: return "COMP_IGUAL";
        case COMP_DISTINTO: return "COMP_DISTINTO";
        case OP_AND: return "OP_AND";
        case OP_OR: return "OP_OR";
        case PAR_IZQ: return "PAR_IZQ";
        case PAR_DER: return "PAR_DER";
        case LLAVE_IZQ: return "LLAVE_IZQ";
        case LLAVE_DER: return "LLAVE_DER";
        case PUNTO_COMA: return "PUNTO_COMA";
        case COMA: return "COMA";
        default: return "TOKEN_DESCONOCIDO";
    }
}

/********************************************************************************
 * 7. ACCIONES SEMÁNTICAS (FUNCIONES f1 A fe)
 * Implementa la lógica asociada a las transiciones del autómata:
 * - f1: Inicializa buffer de ID.
 * - f2: Inicializa buffer de constantes racionales.
 * - f3a: Acumula caracteres en ID aplicando regla D6 (truncamiento a 20 chars).
 * - f3b: Acumula dígitos/barra en racionales con verificación de rango D2 (32 bits).
 * - f4: Reconoce palabras reservadas o inserta ID en la Tabla de Símbolos.
 * - f5: Cierra racional, valida división por cero en compilación e inserta en TS.
 * - f7a, f7b, f7c: Captura de literales de cadena ("...").
 * - f8: Descarte de comentarios multilínea con limpieza de buffer.
 * - fa: Acumulación explícita del 2do carácter en operadores compuestos (==, !=, etc.).
 * - fn: Función nula para separadores/blancos.
 * - fe: Captura y reporte de errores léxicos.
 ********************************************************************************/
void f1(void) { 
    buf_idx = 0; 
    buffer[buf_idx++] = c_actual; 
    buffer[buf_idx] = '\0'; 
}

void f2(void) { 
    buf_idx = 0; 
    val_num_actual = c_actual - '0';
    buffer[buf_idx++] = c_actual; 
    buffer[buf_idx] = '\0'; 
}

void f3a(void) { 
    if (buf_idx < MAX_ID_LEN) { 
        buffer[buf_idx++] = c_actual; 
        buffer[buf_idx] = '\0'; 
    } else if (buf_idx == MAX_ID_LEN) { 
        printf("\n[ADVERTENCIA - Línea %d]: ID '%s...' excede los %d caracteres y fue truncado.\n", num_linea, buffer, MAX_ID_LEN); 
        buf_idx++; 
    }
}

void f3b(void) { 
    if (isdigit(c_actual)) {
        long long digito = c_actual - '0';
        if (val_num_actual <= (MAX_INT_32 - digito) / 10) {
            val_num_actual = val_num_actual * 10 + digito;
        } else {
            val_num_actual = MAX_INT_32 + 1LL; /* Fuerza el estado de overflow sin desbordar C */
            printf("\n[ERROR LEXICO - Linea %d]: Overflow en constante '%s%c' (Excede 32 bits).\n", num_linea, buffer, c_actual);
            error_lexico = 1;
        }
    } else if (c_actual == '/') {
        val_num_actual = 0; /* Reinicia para evaluar el denominador */
    }

    if (buf_idx < MAX_BUFFER - 1) { 
        buffer[buf_idx++] = c_actual; 
        buffer[buf_idx] = '\0'; 
    }
}

void f4(void) {
    if (strcmp(buffer, "def") == 0) ultimo_token_devuelto = PR_DEF;
    else if (strcmp(buffer, "principal") == 0) ultimo_token_devuelto = PR_PRINCIPAL;
    else if (strcmp(buffer, "racional") == 0) ultimo_token_devuelto = PR_RACIONAL;
    else if (strcmp(buffer, "si") == 0) ultimo_token_devuelto = PR_SI;
    else if (strcmp(buffer, "sino") == 0) ultimo_token_devuelto = PR_SINO;
    else if (strcmp(buffer, "mientras") == 0) ultimo_token_devuelto = PR_MIENTRAS;
    else if (strcmp(buffer, "mostrar") == 0) ultimo_token_devuelto = PR_MOSTRAR;
    else if (strcmp(buffer, "retornar") == 0) ultimo_token_devuelto = PR_RETORNAR;
    else if (strcmp(buffer, "and") == 0) ultimo_token_devuelto = OP_AND;
    else if (strcmp(buffer, "or") == 0) ultimo_token_devuelto = OP_OR;
    else {
        if (strlen(buffer) > MAX_ID_LEN) {
            buffer[MAX_ID_LEN] = '\0';
        }
        ultimo_token_devuelto = ID;
        insertar_ts(buffer, "ID", "-", strlen(buffer));
    }
}

void f5(void) {
    char *barra = strchr(buffer, '/');
    /* Evaluación matemática para prevenir divisiones por cero como /0, /00, etc. */
    if (barra != NULL && atoi(barra + 1) == 0) {
        printf("\n[ERROR LEXICO/SEMANTICO - Linea %d]: Division por cero en literal '%s'\n", num_linea, buffer);
        error_lexico = 1;
    }
    
    ultimo_token_devuelto = LIT_RACIONAL;
    
    if (barra == NULL) {
        char val_norm[MAX_BUFFER];
        sprintf(val_norm, "%s/1", buffer);
        insertar_ts(buffer, "LIT_RACIONAL", val_norm, strlen(buffer));
    } else {
        insertar_ts(buffer, "LIT_RACIONAL", buffer, strlen(buffer));
    }
}

void f7a(void) { 
    buf_idx = 0; 
    buffer[buf_idx] = '\0'; 
}


/*
void f7b(void) { 
    if (buf_idx < MAX_BUFFER - 1) { 
        buffer[buf_idx++] = c_actual; 
        buffer[buf_idx] = '\0'; 
    }
}*/



/********NUEVO****/

void f7b(void) { 
    /* ACT 15/10: Si dentro de la cadena encuentra un salto de línea sin haber leído la comilla de cierre */
    if (c_actual == '\n') {
        printf("\n[ERROR LEXICO - Línea %d]: Literal cadena sin cerrar antes del salto de linea.\n", num_linea);
        error_lexico = 1;
        buffer[0] = '\0';
        buf_idx = 0;
        return;
    }

    if (buf_idx < MAX_BUFFER - 1) { 
        buffer[buf_idx++] = c_actual; 
        buffer[buf_idx] = '\0'; 
    }
}


void f7c(void) {
    ultimo_token_devuelto = LIT_CADENA;
    insertar_ts(buffer, "LIT_CADENA", buffer, strlen(buffer));
}

void f8(void) {
    /* Descarte de comentarios: limpia el buffer para no conservar la '/' inicial */
    buf_idx = 0;
    buffer[0] = '\0';
}

/* fa: Acumula el segundo carácter en operadores compuestos (==, !=, <=, >=) */
void fa(void) { 
    if (buf_idx < MAX_BUFFER - 1) { 
        buffer[buf_idx++] = c_actual; 
        buffer[buf_idx] = '\0'; 
    }
}



void fn(void) { 
    if (buf_idx == 0 && c_actual != ' ' && c_actual != '\t' && c_actual != '\r' && c_actual != '\n') {
        buffer[buf_idx++] = c_actual;
        buffer[buf_idx] = '\0';
    }
}

void fe(void) { 
    printf("\n[ERROR LEXICO - Linea %d]: Caracter no permitido '%c'\n", num_linea, c_actual); 
    error_lexico = 1; 
}

/********************************************************************************
 * 8. MATRICES DEL AUTOMÁTA FINITO DETERMINISTA (AFD)
 * Contiene la implementación en C de las tres matrices del diseño:
 * - proceso[][]: Matriz de punteros a funciones de acciones semánticas.
 * - nuevo_estado[][]: Matriz de transiciones de estados (-1 para error/parada, -2 para retracción).
 * - token_por_estado[]: Mapeo directo entre estado final alcanzado y código del token.
 ********************************************************************************/
const AccionSemantica proceso[28][NUM_COLUMNAS] = {
/* e 0 */ { f1,  f2,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fe,  fn,  f7a, fn,  fe },
/* e 1 */ { f3a, f3a, f4,  f4,  f4,  f4,  f4,  f4,  f4,  f4,  f4,  f4,  f4,  f4,  f4,  f4,  f4,  f4,  f4,  fe },
/* e 2 */ { fe,  fe,  fe,  fe,  fe,  fe,  fa,  fe,  fe,  fe,  fe,  fe,  fe,  fe,  fe,  fe,  fe,  fe,  fe,  fe },
/* e 3 */ { fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fe },
/* e 4 */ { f5,  f3b, f5,  f5,  f5,  f3b, f5,  f5,  f5,  f5,  f5,  f5,  f5,  f5,  f5,  f5,  f5,  f5,  f5,  fe },
/* e 5 */ { fe,  f3b, fe,  fe,  fe,  fe,  fe,  fe,  fe,  fe,  fe,  fe,  fe,  fe,  fe,  fe,  fe,  fe,  fe,  fe },
/* e 6 */ { f5,  f3b, f5,  f5,  f5,  f5,  f5,  f5,  f5,  f5,  f5,  f5,  f5,  f5,  f5,  f5,  f5,  f5,  f5,  fe },
/* e 7 */ { fn,  fn,  fn,  fn,  fn,  fn,  fa,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fe },
/* e 8 */ { fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fe },
/* e 9 */ { fn,  fn,  fn,  fn,  fn,  fn,  fa,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fe },
/* e10 */ { fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fe },
/* e11 */ { fn,  fn,  fn,  fn,  fn,  fn,  fa,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fe },
/* e12 */ { fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fe },
/* e13 */ { f7b, f7b, f7b, f7b, f7b, f7b, f7b, f7b, f7b, f7b, f7b, f7b, f7b, f7b, f7b, f7b, f7b, f7c, f7b, fe }, /* COL_OTRO -> fe */
/* e14 */ { fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fe },
/* e15 */ { fn,  fn,  fn,  fn,  f8,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fe },
/* e16 */ { f8,  f8,  f8,  f8,  f8,  f8,  f8,  f8,  f8,  f8,  f8,  f8,  f8,  f8,  f8,  f8,  f8,  f8,  f8,  fe },
/* e17 */ { f8,  f8,  f8,  f8,  f8,  f8,  f8,  f8,  f8,  f8,  f8,  f8,  f8,  f8,  f8,  f8,  f8,  f8,  f8,  fe },
/* e18 */ { fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fe },
/* e19 */ { fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fe },
/* e20 */ { fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fe },
/* e21 */ { fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fe },
/* e22 */ { fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fe },
/* e23 */ { fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fe },
/* e24 */ { fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fe },
/* e25 */ { fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fe },
/* e26 */ { fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fe },
/* e27 */ { fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fn,  fe }
};

const int nuevo_estado[28][NUM_COLUMNAS] = {
/*e0 */ { 1,  4, 25, 26, 27, 15, 11,  9,  7,  2, 21, 22, 23, 24, 18, -1, 19, 13,  0, -1 },
/*e1 */ { 1,  1, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1 },
/*e2 */ {-1, -1, -1, -1, -1, -1,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
/*e3 */ {-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1 },
/*e4 */ {-2,  4, -2, -2, -2,  5, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1 },
/*e5 */ {-1,  6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
/*e6 */ {-2,  6, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1 },
/*e7 */ {-2, -2, -2, -2, -2, -2,  8, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1 },
/*e8 */ {-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1 },
/*e9 */ {-2, -2, -2, -2, -2, -2, 10, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1 },
/*e10*/ {-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1 },
/*e11*/ {-2, -2, -2, -2, -2, -2, 12, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1 },
/*e12*/ {-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1 },
/*e13*/ {13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 13, -1 }, /* COL_OTRO -> -1 */
/*e14*/ {-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1 },
/*e15*/ {-2, -2, -2, -2, 16, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1 },
/*e16*/ {16, 16, 16, 16, 17, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, -1 },
/*e17*/ {16, 16, 16, 16, 16,  0, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, -1 },
/*e18*/ {-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1 },
/*e19*/ {-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1 },
/*e20*/ { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/*e21*/ {-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1 },
/*e22*/ {-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1 },
/*e23*/ {-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1 },
/*e24*/ {-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1 },
/*e25*/ {-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1 },
/*e26*/ {-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1 },
/*e27*/ {-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1 }
};

/* MATRIZ M3: Mapea el último estado del autómata (índice 0-27) con el código de token reconocido (265-286). 
 * Los valores en 0 representan estados intermedios (no finales). */
/*
static const int token_por_estado[28] = {
    0, 265, 0, 278, 266, 0, 266, 273, 275, 274, 276,
    268, 277, 0, 267, 272, 0, 0, 285, 286, 0, 
    281, 282, 283, 284, 269, 270, 271
};	*/



/*CAMBIO: 28/10*/
/* AHORA (Constantes simbólicas de Bison) */
/* MATRIZ M3: Mapea el último estado del autómata (índice 0-27) con el código de token reconocido (265-286). 
 * Los valores en 0 representan estados intermedios (no finales). */
static const int token_por_estado[28] = {
    0, ID, 0, COMP_DISTINTO, LIT_RACIONAL, 0, LIT_RACIONAL, COMP_MAYOR, COMP_MAYOR_IGUAL, COMP_MENOR, COMP_MENOR_IGUAL,
    OP_ASIG, COMP_IGUAL, 0, LIT_CADENA, OP_DIV, 0, 0, PUNTO_COMA, COMA, 0, 
    PAR_IZQ, PAR_DER, LLAVE_IZQ, LLAVE_DER, OP_SUMA, OP_RESTA, OP_MULT
};


/********************************************************************************
 * 9. MOTOR DEL ANALIZADOR LÉXICO (FUNCIÓN yylex)
 * Estructura de Doble Bucle (Basada en la arquitectura de 3 Matrices de AFD):
 * - Bucle Externo: Avanza hasta reconocer 1 token o llegar al fin de archivo (EOF).
 * - Bucle Interno: Camina por el autómata carácter por carácter consumiendo la matriz.
 * - Retracción: Devuelve el último carácter (Lookahead) al archivo al salir del autómata.
 * - Clasificación M3: Asigna el token final alcanzado y resuelve la ambigüedad ID/PR.
 ********************************************************************************/
int yylex(void) {
    int c;
    int estado, estado_ant, columna;
    int token_reconocido = 0;

    /* --------------------------------------------------------------------------
     * 1. BUCLE EXTERNO: Controla la lectura general del archivo.
     * Cada iteración completa de este bucle debe reconocer y retornar UN TOKEN.
     * -------------------------------------------------------------------------- */
    while ((c = fgetc(inputFile)) != EOF) {
        
        /* Omitir espacios en blanco, tabulaciones y saltos de línea iniciales */
        if (c == ' ' || c == '\t' || c == '\r') continue;
        if (c == '\n') { 
            num_linea++; 
            continue; 
        }

        /* Inicialización del estado del autómata para empezar a construir un nuevo lexema */
        estado = 0;
        estado_ant = 0;
        buf_idx = 0;
        buffer[0] = '\0';

        /* --------------------------------------------------------------------------
         * 2. BUCLE INTERNO: El caminante del autómata (Caminata de Transiciones).
         * Consume carácter por carácter usando M1 (nuevo_estado) y M2 (proceso)
         * hasta chocar contra un Estado de Parada (estado < 0) o el fin de archivo.
         * -------------------------------------------------------------------------- */
        while (estado >= 0) {
            c_actual = (char)c;       // Guarda el carácter actual.
			columna = get_evento(c);  // Obtiene la columna correspondiente al carácter.

            /* MATRIZ M2 (proceso): Ejecuta la acción semántica asociada */
            (*proceso[estado][columna])();
            
            
            /********NUEVO:ACT 15/10:****/
           /* RECUPERACION DE CADENA NO CERRADA:
             * Si estando en estado 13 (cadena) lee un salto de linea, corta la caminata */
            if (estado == 13 && c_actual == '\n') {
                break;
            }             

            /* Guarda el estado actual antes de saltar (necesario para consultar M3 al salir) */
            estado_ant = estado;
            
            /* MATRIZ M1 (nuevo_estado): Transición hacia el siguiente estado del autómata */
            estado = nuevo_estado[estado][columna];

          	if (estado < 0) break; /* Estado de parada o retracción, break sale del while  */

            c = fgetc(inputFile); 	/*lee sig caracter*/
            if (c == '\n') num_linea++;
            
        }

		/* RECUPERACIÓN DE ERROR LÉXICO:
		 * Si en el estado inicial (0) se lee un carácter inválido (como '$' o '@'), 
		 * se descarta y se salta al siguiente carácter para continuar analizando. */
         if (estado == -1 && estado_ant == 0) {
            continue; //continue descarta esta iteración del while exterior y pasa a la siguiente SIN RETRACCIÓN 
            		  //ya que esa parte no llega a ejecutarse en esta iteración
        }

	    /* --------------------------------------------------------------------------
		* RETRACCIÓN DE ULTIMO CARÁCTER LEÍDO  
	    /* -------------------------------------------------------------------------- */			
		if (c != EOF){ // Si no llegó al final del archivo
	    ungetc(c, inputFile); // Devuelve el carácter leído para procesarlo después
	    if (c == '\n') num_linea--; // Corrige el contador si era un salto de línea
		}

        /* --------------------------------------------------------------------------
         * 4. CONSULTA EN MATRIZ M3 (token_por_estado):
         * Obtiene el código de Token asociado al último estado válido alcanzado.
         * -------------------------------------------------------------------------- */
        token_reconocido = token_por_estado[estado_ant];

        /* --------------------------------------------------------------------------
         * 5. RESOLUCIÓN DE AMBIGÜEDAD (Palabra Reservada vs Identificador):
         * -------------------------------------------------------------------------- */
        if (token_reconocido == ID) { // Si el token reconocido es un identificador
            f4(); /* Determina si es Palabra Reservada o ID y actualiza ultimo_token_devuelto */
            token_reconocido = ultimo_token_devuelto; // el token definitivo
        }

        /* MOSTRAR O PROCESAR TOKEN */
        if (token_reconocido != 0) {
            printf("[Linea %2d] | %-18s | %-8d | '%s'\n",
                   num_linea,
                   obtener_nombre_token(token_reconocido),
                   token_reconocido,
                   buffer);
            return token_reconocido; /* Entrega el token a Bison */
        }
    }

    /* Fin de archivo alcanzado */
    return 0;
}

/********************************************************************************
 * 10. EXPORTACIÓN DE LA TABLA DE SÍMBOLOS
 * Muestra por consola y graba en el archivo plano "tabla_simbolos.txt" la
 * estructura final recopilada durante el análisis del código fuente.
 ********************************************************************************/
void exportarTS(void) {
    FILE *f_ts = fopen("tabla_simbolos.txt", "w");
    
    printf("\n=================================================================\n");
    printf("                  TABLA DE SIMBOLOS GENERADA                     \n");
    printf("=================================================================\n");
    printf("%-20s | %-15s | %-20s | %-8s\n", "NOMBRE", "TIPO", "VALOR", "LONGITUD");
    printf("-----------------------------------------------------------------\n");

    if (f_ts != NULL) {
        fprintf(f_ts, "NOMBRE,TIPO,VALOR,LONGITUD\n");
    }

    for (int i = 0; i < cant_simbolos; i++) {
        printf("%-20s | %-15s | %-20s | %-8d\n", 
               tabla_simbolos[i].nombre, 
               tabla_simbolos[i].tipo, 
               tabla_simbolos[i].valor, 
               tabla_simbolos[i].longitud);

        if (f_ts != NULL) {
            fprintf(f_ts, "%s,%s,%s,%d\n", 
                    tabla_simbolos[i].nombre, 
                    tabla_simbolos[i].tipo, 
                    tabla_simbolos[i].valor, 
                    tabla_simbolos[i].longitud);
        }
    }

    if (f_ts != NULL) {
        fclose(f_ts);
        printf("\n[TS]: Exportada exitosamente al archivo 'tabla_simbolos.txt'\n");
    }
}

/********************************************************************************
 * 11. PUNTO DE ENTRADA Y PRUEBA (FUNCIÓN main)
 * Abre el programa fuente ("ejemplo.raz"), ejecuta el scanner en bucle emitiendo
 * cada token reconocido hasta llegar al final del archivo, e invoca la
 * exportación de la Tabla de Símbolos si no ocurrieron errores léxicos.
 ********************************************************************************/ 
 
 
 /*
int main(int argc, char *argv[]) {
    // Abre el archivo fuente en modo lectura
    if ((inputFile = fopen("ejemplo.raz", "r")) == NULL) {
        printf("Error al abrir el archivo 'ejemplo.raz'\n");
        return 1; // Finaliza si no pudo abrir el archivo
    }

    // Muestra el encabezado del reporte léxico
    printf("=================================================================================\n");
    printf("           REPORTE DE ANALIZADOR LÉXICO - COMPILADOR RAZIO (GRUPO C)             \n");
    printf("=================================================================================\n");
    printf("%-8s | %-18s | %-8s | %-30s\n", "LÍNEA", "TOKEN", "CÓDIGO", "LEXEMA");
    printf("---------------------------------------------------------------------------------\n");
*/
    /* 2. Se llama a yylex() UNA SOLA VEZ (el bucle externo de yylex procesa todo) */
//yylex();

    /* 3. Cierra el archivo al terminar */
	//fclose(inputFile);

    /* Reporte de estado */
    /*
    if (error_lexico == 0) {
        printf("\n - Compilación AL EXITOSA - \n");
        exportarTS();
    } else {
        printf("\n - Análisis Léxico completo con ERRORES - \n");
    }

    return 0;
    
      
}
*/
