# EJERCICIOS DAG

## Ejercicio 1 — Compute T1, T∞, and Tp (p=2,4,8) for the DAG below.

Asumiendo 13 nodos (cada tarea = 1 unidad de tiempo):

- **T1** = 13 (work = suma de todos los nodos)
- **T∞** = 4 (span = camino crítico más largo)

Usando Brent's Lemma: `max(T1/p, T∞) <= Tp <= (T1 - T∞)/p + T∞`

| p | Lower bound `max(T1/p, T∞)` | Upper bound `(T1-T∞)/p + T∞` |
|---|----------------------------|-------------------------------|
| 2 | max(13/2, 4) = **6.5**     | (13-4)/2 + 4 = **8.5**        |
| 4 | max(13/4, 4) = **4.0**     | (13-4)/4 + 4 = **6.25**       |
| 8 | max(13/8, 4) = **4.0**     | (13-4)/8 + 4 = **5.125**      |

---

## Ejercicio 2 — DAG de Fibonacci f(5)

A system recursively creates a task to compute f(i-1), another to compute f(i-2), and one to merge the two results.

### Work T1

Cada tarea (spawn o merge) toma 1 unidad de tiempo.

```
W(n) = W(n-1) + W(n-2) + 2   (el +2 es por spawn y merge propios)
W(0) = 1
W(1) = 1
W(2) = W(1) + W(0) + 2 = 1 + 1 + 2 = 4
W(3) = W(2) + W(1) + 2 = 4 + 1 + 2 = 7
W(4) = W(3) + W(2) + 2 = 7 + 4 + 2 = 13
W(5) = W(4) + W(3) + 2 = 13 + 7 + 2 = 22
```

**T1 = 22**

### Span T∞

El camino más largo siempre pasa por f(i-1) (rama dominante).

```
S(n) = S(n-1) + 2   (spawn propio + rama f(i-1) + merge)
S(0) = 1
S(1) = 1
S(2) = S(1) + 2 = 3
S(3) = S(2) + 2 = 5
S(4) = S(3) + 2 = 7
S(5) = S(4) + 2 = 9
```

**T∞ = 9**

### Estructura del DAG de f(5)

Camino crítico (T∞ = 9 nodos):

`f(5) → f(4) → f(3) → f(2) → f(1) → merge_2 → merge_3 → merge_4 → merge_5`
