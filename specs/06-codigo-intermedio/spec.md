# Spec — 06-codigo-intermedio

## Objetivo

## Alcance

## Entradas / Salidas

## Reglas y decisiones de diseño

## Criterios de aceptación


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
