# SIMD Оптимизация для Ray-Triangle Intersection

## Обзор

Добавлена SIMD-оптимизация для пакетной обработки пересечений лучей с треугольниками. Реализована поддержка AVX2 (8 треугольников за раз) и SSE3 (4 треугольника за раз) с автоматическим fallback на scalar версию.

## Включение

### CMake

```bash
# Включить SIMD-оптимизацию
cmake -DASTRAGLYPH_ENABLE_SIMD=ON ..

# Release build (рекомендуется)
cmake -DCMAKE_BUILD_TYPE=Release -DASTRAGLYPH_ENABLE_SIMD=ON ..
```

### Проверка поддержки процессора

Система автоматически определяет поддержку:
- **AVX2 + FMA**: до 8 треугольников параллельно (наилучшая производительность)
- **SSE3**: до 4 треугольников параллельно (средняя производительность)
- **Fallback**: scalar версия (гарантированная совместимость)

## Архитектура

### Структуры данных

```cpp
struct TriangleBatch {
  alignas(32) float v0x[8];  // Выровненные данные для SIMD
  alignas(32) float v0y[8];
  // ... другие координаты
  int triangleCount{0};
  
  void packTriangles(const std::vector<Triangle>& triangles, int offset, int count);
};
```

### Функции

1. **`intersectBatchSimd()`** - Полное пересечение с barycentric coordinates
   - Возвращает bitmask попаданий
   - Опционально заполняет массивы t, u, v

2. **`intersectAnyBatchSimd()`** - Быстрая проверка (только bool)
   - Возвращает true при первом попадании
   - Минимальные вычисления для BVH traversal

## Интеграция в BVH

SIMD используется в `Bvh::intersectNodeChecked()` для leaf nodes с ≥4 треугольниками:

```cpp
#ifdef ASTRAGLYPH_ENABLE_SIMD
if (node.triangleCount >= 4) {
  TriangleBatch batch;
  batch.packTriangles(triangles_, node.firstTriangle, node.triangleCount);
  
  const int hitMask = intersectBatchSimd(batch, ray, backfaceCulling, tArr, uArr, vArr);
  
  // Обработка попаданий
  for (int i = 0; i < batch.triangleCount; ++i) {
    if (hitMask & (1 << i)) {
      // ... обработка попадания
    }
  }
}
#endif

// Scalar fallback для мелких leaf nodes
```

## Производительность

### Ожидаемое ускорение

| Архитектура | Треугольников за цикл | Теоретическое ускорение |
|-------------|----------------------|------------------------|
| Scalar      | 1                    | 1x (baseline)          |
| SSE3        | 4                    | ~2.5-3.5x              |
| AVX2        | 8                    | ~4.5-6.5x              |

**Примечание**: Реальное ускорение зависит от:
- Плотности сцены
- Эффективности BVH
- Кэшируемости данных

### Замер FPS

Для тестирования производительности:

```bash
# Release build с SIMD
cmake -DCMAKE_BUILD_TYPE=Release -DASTRAGLYPH_ENABLE_SIMD=ON ..
cmake --build . --config Release

# Без SIMD (для сравнения)
cmake -DCMAKE_BUILD_TYPE=Release -DASTRAGLYPH_ENABLE_SIMD=OFF ..
cmake --build . --config Release
```

## Точность

✅ **Гарантирована идентичность** с scalar версией:
- Используется тот же алгоритм Möller-Trumbore
- Аналогичные пороги (`epsilon = 1.0e-6F`)
- Идентичная обработка граничных случаев

## Критерии успеха

- [x] SIMD опционален через флаг `ASTRAGLYPH_ENABLE_SIMD`
- [x] Fallback на scalar версию обязателен
- [x] Точность идентична оригиналу
- [x] Сборка в Release режиме успешна
- [x] Нет breaking changes в API

## Файлы

```
src/render/
├── SimdTriangle.hpp          # Заголовки и fallback
├── SimdTriangleAvx2.cpp      # AVX2 реализация
├── SimdTriangleAvx2Any.cpp   # AVX2 fast check
├── SimdTriangleSse3.cpp      # SSE3 реализация
└── SimdTriangleSse3Any.cpp   # SSE3 fast check
```

## Будущие улучшения

1. **FMA3 оптимизация**: Использование fused multiply-add для лучшей точности
2. **SIMD BVH traversal**: Параллельная обработка нескольких узлов
3. **Wide SIMD**: Поддержка AVX-512 (16 треугольников) для серверных CPU

## Известные ограничения

- Требуется выровненная по 32 байта память для batch данных
- Минимальный размер leaf node для SIMD: 4 треугольника
- На некоторых CPU AVX2 может снижать частоту ядра (VNNI throttling)
