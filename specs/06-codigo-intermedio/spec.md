# Spec — 06-codigo-intermedio

## Objetivo

## Alcance

## Entradas / Salidas

## Reglas y decisiones de diseño

## Criterios de aceptación



---

# Especificación de Generación de Código Intermedio: Tercetos

## 1. Estructura de Datos en C

La representación intermedia del compilador de **Razio** se basa en la generación de **tercetos**. Cada terceto abstrae una operación elemental del programa mediante una estructura compuesta por tres campos: **operación**, **operando 1** y **operando 2**.

Para gestionar esta representación en memoria durante el análisis, se define una estructura global en C dentro de `sintactico.y` (o en su cabecera `tercetos.h`):

```c
typedef struct {
    char operacion[15]; // Tipo de instrucción (ej. "+", "=", "BF", "CMP")
    char operando1[50]; // Variable, literal o referencia a terceto previo (ej. "[T0]")
    char operando2[50]; // Variable, literal, referencia previo o "_" si es unario
} Terceto;

/* Arreglo global para almacenar los tercetos y contador de índice */
Terceto tabla_tercetos[1000];
int indice_terceto = 0;

```

### Mecánica de Referenciación

Los resultados de operaciones complejas no generan variables temporales explícitas en el código fuente. En su lugar, se utiliza el índice asignado en la tabla de tercetos formateado como **`[T_]`**. Este valor actúa como identificador de operando en tercetos posteriores.

---

## 2. Conjunto de Operadores Soportados

La tabla de tercetos soporta un conjunto cerrado de instrucciones intermedias que simplifican el código fuente para su posterior traducción a lenguaje ensamblador:

* **Operadores Aritméticos:** `+`, `-`, `*`, `/`
* Realizan operaciones elementales binarias entre dos operandos.


* **Operadores de Asignación:** `=`
* Asignan el valor del operando 2 (variable, literal o terceto) al operando 1 (variable).


* **Operadores de Comparación:** `>`, `<`, `>=`, `<=`, `==`, `!=`
* Comparan dos expresiones para preparar una evaluación lógica.


* **Control de Flujo (Saltos):**
* `BF` (*Branch on False*): Salto por Falso. Si el resultado de la condición (operando 1) es falso, interrumpe la ejecución lineal y salta a la posición especificada (operando 2).
* `BI` (*Branch Inconditional*): Salto Incondicional. Fuerza el salto a una posición específica de la tabla de tercetos (operando 1) sin evaluar condición.


* **Entrada y Salida (E/S):** `MOSTRAR`, `LEER`
* Gestionan la impresión o lectura de datos en pantalla/teclado.



---

## 3. Generación de Tercetos en Bison

La emisión de tercetos se realiza mediante **Traducción Dirigida por la Sintaxis (SDT)** dentro de las **acciones semánticas** `{ ... }` de `sintactico.y`.

Bison utiliza atributos sintetizados para comunicarse entre reglas:

* **`$$`**: Representa el valor que **devuelve** la regla actual hacia la regla gramatical superior (usualmente la referencia `"[T_]"` o un identificador/literal).
* **`$1`, `$2`, `$3**`: Corresponden a los valores que aportan los elementos individuales reconocidos del lado derecho de la producción (de izquierda a derecha).

### Ejemplo de integración en Bison:

```yacc
expresion:
    expresion OP_SUMA termino
    {
        /* 1. Genera el terceto de la suma con los valores de $1 y $3 */
        int pos = crear_terceto("+", $1, $3);

        /* 2. Asigna la referencia [T_] producida a $$ para las reglas superiores */
        sprintf($$, "[T%d]", pos);
    }
;

```

---

## 4. Traducción de las Estructuras del Lenguaje Razio

A continuación se detalla cómo se transforman las distintas construcciones gramaticales de Razio en secuencias de tercetos:

### A. Asignaciones y Expresiones Aritméticas

Las expresiones complejas se descomponen en operaciones binarias según la prioridad de los operadores.

* **Código fuente en Razio:**
```c
x = a + 5 / 2;

```


* **Generación de Tercetos:**
```text
[T0] = (/, 5, 2)
[T1] = (+, a, [T0])
[T2] = (=, x, [T1])

```



---

### B. Estructura Condicional (`si`)

Para la estructura `si`, se emite una comparación seguida de un salto por falso `BF`. Como el destino del salto no se conoce al momento de evaluar la condición, se aplica la técnica de **Backpatching (Descompletado y Completado)** para rellenar la dirección al salir del bloque.

* **Código fuente en Razio:**
```c
si (x > 0) {
    y = 1;
}

```


* **Generación de Tercetos:**
```text
[T0] = (>, x, 0)
[T1] = (BF, [T0], [T3])    <-- El destino [T3] se completa al cerrar la llave del bloque
[T2] = (=, y, 1)
[T3] = ...                 <-- Continúa la ejecución del programa

```



---

### C. Estructura Iterativa (`mientras`)

En el caso de un bucle `mientras`, además del salto condicional `BF` para salir de la iteración cuando la condición deja de cumplirse, se utiliza un salto incondicional `BI` al final del bloque para volver a evaluar la condición.

* **Código fuente en Razio:**
```c
mientras (x < 10) {
    x = x + 1;
}

```


* **Generación de Tercetos:**
```text
[T0] = (<, x, 10)          <-- Inicio de evaluación de condición
[T1] = (BF, [T0], [T5])    <-- Si es falso, salta fuera del bucle a [T5]
[T2] = (+, x, 1)
[T3] = (=, x, [T2])
[T4] = (BI, [T0], _)       <-- Vuelve a evaluar la condición
[T5] = ...                 <-- Fin del bucle

```
