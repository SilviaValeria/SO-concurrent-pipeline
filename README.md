# Pipeline Concurrente de Procesamiento de Datos
## Proyecto de Sistemas Operativos- Silvia Valeria Bernal Aradillas
---
## Descripción General

Este proyecto implementa la simulación de un pipeline concurrente de procesamiento de datos utilizando C++ y primitivas de sincronización propias de los sistemas operativos modernos.

El sistema modela un flujo de procesamiento compuesto por tres etapas ejecutadas en paralelo mediante hilos independientes:

1. Generador de datos (Producer)
2. Procesador de datos (Processor)
3. Escritura en archivo (Writer)

Cada etapa se comunica mediante buffers acotados (colas con capacidad limitada), garantizando sincronización correcta y control de flujo.
---
## Conceptos de Sistemas Operativos Aplicados

- Creación y gestión de hilos (`std::thread`)
- Exclusión mutua (`std::mutex`)
- Variables de condición (`std::condition_variable`)
- Problema clásico Productor–Consumidor
- Implementación de buffers acotados (Bounded Buffer)
- Backpressure (propagación de bloqueo entre etapas)
- Cuellos de botella (bottlenecks)
- Comparación entre ejecución secuencial y concurrente
- Medición de rendimiento: latencia, throughput y speedup
---
## Arquitectura del Sistema
+-----------+ +-------------+ +------------+
| Generador | ---> | Buffer 1 | ---> | Procesador |
+-----------+ | (Acotado) | +------------+
| ^
| |
v |
+-------------+ +----------+
| Buffer 2 | ---> | Escritor |
+-------------+ +----------+

Los buffers están diseñados para:
- Bloquear la inserción cuando están llenos.
- Bloquear la extracción cuando están vacíos.

Este mecanismo evita condiciones de carrera y elimina el uso de espera activa (busy waiting).
---
## Compilación y Ejecución

Compilar el proyecto: make build

Ejecutar el programa: make run

Guardar resultados en archivo: make results
---
##  Métricas Evaluadas

El sistema mide y compara:

- Tiempo total de ejecución (ms)
- Speedup (Secuencial / Concurrente)
- Throughput (trabajos por segundo)
- Latencia promedio
- Latencia p95
- Tiempo bloqueado por saturación de buffers
---
## Estructura del Proyecto
so_pipeline/
│
├── Makefile
├── README.md
├── results.txt
│
├── src/
│ └── main.cpp
│
├── report/
│ └── (documentación final en PDF)
│
└── diagrams/
---
## Objetivo Académico

El objetivo de este proyecto es demostrar el comportamiento real de un sistema concurrente basado en pipeline, evidenciando:

- Sincronización correcta entre hilos
- Efecto del tamaño del buffer en el rendimiento
- Identificación de cuellos de botella
- Propagación de backpressure
- Diferencias entre ejecución secuencial y concurrente
- Trade-off entre throughput y latencia

