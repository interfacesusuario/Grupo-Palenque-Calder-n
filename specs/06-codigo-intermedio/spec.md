# Spec — 06-codigo-intermedio

## 1. Objetivo
Esta etapa permite transformar las instrucciones del programa en una serie de operaciones más simples y ordenadas. Los tercetos sirven como paso intermedio entre el análisis del código fuente y la posterior generación del código ensamblador.

## 2. Alcance
Definición de qué tareas se van a realizar en esta etapa del compilador y cuáles quedan fuera.

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

**Código fuente**: las instrucciones de Razio que Bison reconoce utilizando las reglas gramaticales definidas en sintactico.y.

**Datos de las instrucciones**: los valores necesarios para representar las operaciones, como nombres de variables, números y expresiones matemáticas.

**Tabla de Símbolos**: la información que permite verificar si las variables utilizadas fueron declaradas previamente.

**Salidas:**

**Tabla de tercetos en memoria:** las operaciones se almacenan en tabla_tercetos[] a medida que se procesa el programa.

**Archivo tercetos.txt:** al finalizar el proceso, los tercetos se guardan en un archivo de texto, donde cada uno representa una operación mediante un operador y dos operandos.

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

