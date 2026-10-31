# Spec — 06-codigo-intermedio

## 1. Objetivo

Definir cómo se genera el código intermedio mediante tercetos en el compilador de Razio.

Esta etapa permite transformar las instrucciones del programa en una serie de operaciones más simples y ordenadas. Los tercetos sirven como paso intermedio entre el análisis del código fuente y la posterior generación del código ensamblador.

## 2. Alcance

**Incluye:**

- Definición de la estructura utilizada en C para almacenar los tercetos.
- Generación de tercetos para operaciones aritméticas (`+`, `-`, `*`, `/`) y asignaciones (`=`).
- Representación de las estructuras condicionales (`si` / `sino`) e iterativas (`mientras`).
- Generación de instrucciones para la función de salida `mostrar`.
- Resolución de los destinos de los saltos mediante la técnica de *Backpatching*.
- Exportación de los tercetos generados a un archivo llamado `tercetos.txt`.

**No incluye:**

- Generación del código ensamblador o del código ejecutable final.
- Optimización de los tercetos, como simplificar operaciones o eliminar instrucciones innecesarias.

## 3. Entradas y salidas

**Entradas:**

- Las reglas gramaticales reconocidas por Bison en el archivo `sintactico.y`.
- Los valores obtenidos durante el reconocimiento de las reglas, como operandos, variables y expresiones.
- La información de la Tabla de Símbolos, que permite comprobar si las variables utilizadas fueron declaradas previamente.

**Salidas:**

- **En memoria:** una tabla llamada `tabla_tercetos[]`, donde se almacenan las operaciones generadas durante el análisis.
- **En archivo:** un archivo `tercetos.txt` que contiene todos los tercetos ordenados, utilizando el formato `[T_] (Operador, Operando 1, Operando 2)`.

## 4. Reglas y decisiones de diseño

### 4.1. Estructura de los tercetos y almacenamiento

Cada terceto representa una operación y contiene tres campos: la operación y sus dos operandos.

Para almacenar esta información se utiliza la siguiente estructura en C:

```c
typedef struct {
    char operacion[15]; // Operación a realizar (+, =, BF, BI, etc.)
    char operando1[50]; // Primer operando
    char operando2[50]; // Segundo operando
} Terceto;

Terceto tabla_tercetos[1000];
int indice_terceto = 0;
```

La tabla permite guardar los tercetos a medida que se reconocen las distintas instrucciones del programa.

### 4.2. Uso de referencias en lugar de variables temporales

No se crean variables adicionales para almacenar los resultados de las operaciones intermedias.

En su lugar, cada resultado se identifica mediante la posición del terceto que lo generó. Por ejemplo, `[T0]` representa el resultado del primer terceto y puede utilizarse en operaciones posteriores.

### 4.3. Integración con Bison

La generación de tercetos se realiza dentro de las acciones asociadas a las reglas gramaticales del archivo `sintactico.y`.

Cuando Bison reconoce una expresión u otra instrucción del lenguaje, se ejecuta la acción correspondiente y se genera el terceto necesario.

Para trabajar con los valores de cada regla se utilizan las referencias `$1`, `$2`, `$3`, etc., que permiten acceder a sus distintos elementos. Por otro lado, `$$` permite guardar el resultado de la regla completa.

De esta manera, las referencias a los tercetos generados pueden utilizarse en otras expresiones e instrucciones.

### 4.4. Manejo de los tipos de datos

En Razio se utiliza el tipo de dato `racional` para las variables.

Por este motivo, las operaciones aritméticas trabajan con valores de ese tipo. Durante la generación de tercetos también se comprueba, mediante la Tabla de Símbolos, que las variables utilizadas hayan sido declaradas previamente.

### 4.5. Operadores disponibles

Los tercetos contemplan las siguientes operaciones:

- **Aritméticas y asignación:** `+`, `-`, `*`, `/`, `=`.
- **Comparaciones:** `>`, `<`, `>=`, `<=`, `==`, `!=`.
- **Saltos:**
  - `BF`: permite saltar a otra posición cuando una condición es falsa.
  - `BI`: permite saltar a otra posición sin evaluar ninguna condición.
- **Entrada y salida:** `MOSTRAR`.

### 4.6. Resolución de saltos (Backpatching)

En las estructuras condicionales e iterativas, al generar un salto no siempre se conoce cuál será la posición exacta a la que debe dirigirse.

Para resolverlo se utiliza una técnica llamada *Backpatching*, que consiste en completar el destino del salto más adelante, cuando ya se conoce esa posición.

El procedimiento es el siguiente:

1. Se genera el terceto del salto, dejando pendiente la posición de destino.
2. Se guarda la posición de ese terceto en una estructura auxiliar.
3. Cuando se conoce el lugar al que debe saltar, se recupera el terceto pendiente y se completa su destino.

De esta manera, todos los saltos quedan correctamente relacionados con las posiciones correspondientes de la tabla.

## 5. Criterios de aceptación

Para considerar que esta etapa funciona correctamente, se deben cumplir las siguientes condiciones:

1. **Orden correcto de las operaciones:** los tercetos deben generarse respetando la prioridad de los operadores. Por ejemplo, en `x = a + b * c`, primero debe generarse la multiplicación y después la suma.

2. **Formato del archivo:** cuando el análisis del programa finaliza correctamente, se debe generar el archivo `tercetos.txt`, con cada terceto en una línea y siguiendo el formato `[T_] (op, op1, op2)`.

3. **Saltos correctamente resueltos:** las instrucciones `BF` y `BI` deben tener indicado un destino válido dentro de la tabla de tercetos.

4. **Control de errores:** si el programa contiene errores léxicos o sintácticos (`error_lexico != 0` o `error_sintactico != 0`), no se debe generar ni exportar el archivo de tercetos.
















# Especificación de Generación de Código Intermedio: Tercetos

## 1. Estructura de datos en C

El compilador de Razio utiliza tercetos para representar las operaciones del programa antes de generar el código ensamblador.

Cada terceto está formado por tres campos: la operación que se debe realizar y los dos operandos que participan en ella.

Para almacenar los tercetos durante la compilación, se utiliza la siguiente estructura en C:

```c
typedef struct {
    char operacion[15];  // Operación a realizar: +, -, *, /, =, BF, etc.
    char operando1[50];  // Primer operando
    char operando2[50];  // Segundo operando
} Terceto;

Terceto tabla_tercetos[1000];
int indice_terceto = 0;
```

Los resultados de las operaciones se identifican mediante una referencia a la posición del terceto que los generó. Por ejemplo, `[T0]` representa el resultado del primer terceto y puede utilizarse como operando en una operación posterior.

## 2. Operadores soportados

Los tercetos permiten representar las distintas operaciones que se pueden realizar en Razio. Se contemplan los siguientes grupos:

- **Aritméticos:** `+`, `-`, `*`, `/`. Permiten realizar sumas, restas, multiplicaciones y divisiones.
- **Asignación:** `=`. Permite asignar un valor a una variable.
- **Comparación:** `>`, `<`, `>=`, `<=`, `==`, `!=`. Permiten comparar dos valores.
- **Saltos:** permiten modificar el orden de ejecución del programa.
  - `BF`: realiza un salto cuando una condición es falsa.
  - `BI`: realiza un salto incondicional, sin necesidad de evaluar una condición.
- **Entrada y salida:** `MOSTRAR` y `LEER`. Permiten mostrar información y recibir datos.

## 3. Generación de tercetos en Bison

La generación de tercetos se realiza desde las reglas gramaticales del archivo `sintactico.y`, mediante acciones que se ejecutan cuando Bison reconoce una determinada construcción del lenguaje.

Para esto, Bison utiliza algunas referencias que permiten acceder a los valores de los elementos de cada regla:

- `$1`, `$2`, `$3`: permiten obtener los valores de los elementos que forman parte de una regla, según su posición.
- `$$`: representa el resultado de la regla completa. Ese resultado puede utilizarse posteriormente en otras reglas.

Por ejemplo, al reconocer una suma como `a + 5`, se toman los valores de ambos operandos y se genera un terceto que representa esa operación. Luego, su referencia se guarda en `$$` para que pueda utilizarse en operaciones posteriores.

De esta manera, los tercetos se van generando a medida que el compilador reconoce las distintas expresiones e instrucciones del programa.

## 4. Traducción de las estructuras de Razio a tercetos

En esta sección se muestran ejemplos de cómo se representan las distintas instrucciones del lenguaje mediante tercetos.

### A. Asignaciones y expresiones aritméticas

Cuando aparece una expresión que contiene varias operaciones, se genera un terceto por cada operación, respetando la prioridad de los operadores.

**Código fuente en Razio:**

```c
x = a + 5 / 2;
```

**Tercetos generados:**

```text
[T0] = (/, 5, 2)
[T1] = (+, a, [T0])
[T2] = (=, x, [T1])
```

Primero se realiza la división entre 5 y 2. Luego, su resultado se utiliza para realizar la suma con `a`. Finalmente, el resultado se asigna a `x`.

### B. Estructura condicional (`si`)

Para representar una condición, primero se genera un terceto que realiza la comparación. Luego se utiliza `BF` para indicar a qué posición debe continuar la ejecución si la condición es falsa.

**Código fuente en Razio:**

```c
si (x > 0) {
    y = 1;
}
```

**Tercetos generados:**

```text
[T0] = (>, x, 0)
[T1] = (BF, [T0], [T3])
[T2] = (=, y, 1)
[T3] = ...
```

Primero se compara `x` con `0`. Si la condición es falsa, se salta directamente a `[T3]`, evitando ejecutar la asignación. Si es verdadera, se continúa con la instrucción dentro del bloque.

El destino del salto se completa cuando el compilador conoce la posición correspondiente. Este procedimiento se conoce como *Backpatching*.

### C. Estructura iterativa (`mientras`)

Para representar un ciclo `mientras`, se genera primero la condición que determina si se debe ejecutar el bloque. Si la condición es falsa, se salta fuera del ciclo. Al finalizar el bloque, se utiliza un salto incondicional para volver a evaluar la condición.

**Código fuente en Razio:**

```c
mientras (x < 10) {
    x = x + 1;
}
```

**Tercetos generados:**

```text
[T0] = (<, x, 10)
[T1] = (BF, [T0], [T5])
[T2] = (+, x, 1)
[T3] = (=, x, [T2])
[T4] = (BI, [T0], _)
[T5] = ...
```

Primero se evalúa si `x` es menor que 10. Si la condición es falsa, se sale del ciclo. Si es verdadera, se ejecuta la suma y se actualiza el valor de `x`. Al finalizar, se vuelve al inicio para evaluar nuevamente la condición.

---

## Resumen

La generación de código intermedio mediante tercetos permite representar las instrucciones de Razio de una manera ordenada y sencilla. Esta representación sirve como paso intermedio entre el código fuente y la posterior generación de código ensamblador.
