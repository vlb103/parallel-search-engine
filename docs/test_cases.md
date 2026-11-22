# Тестові запити

Датасет описано в README. Очікуваний результат для рідкісного слова отримано незалежно від системи через `grep`: слово `zymurgy` є рівно в 5 файлах.

| № | Запит | Очікуваний результат | Фактичний результат | Статус |
|---|---|---|---|---|
| 1 | `zymurgy` | ті самі 5 файлів, що дає `grep -rlwi zymurgy` | 5 результатів: `Charlie_Papazian`, `Encyclopedia_Magica`, `Beer_Judge_Certification_Program`, `List_of_United_States_magazines`, `Two_Hearted_River`. Збігаються з `grep` | пройдено |
| 2 | `ZYMURGY` | ті самі файли, що в тесті 1 | ті самі 5 файлів з тими самими оцінками | пройдено |
| 3 | `zymurgy beer` | першим іде документ з обома словами | 1. `Charlie_Papazian` (zymurgy 3 рази, beer 15), 2. `Beer_Judge_Certification_Program` (обидва слова), далі статті лише з `beer` | пройдено |
| 4 | `qwertyzxcv` | порожній `results` | `{"results":[],"time_ms":2}` | пройдено |
| 5 | `the of and` | порожній `results` | `{"results":[],"time_ms":0}` | пройдено |
| 6 | `zymurgy!!!` | ті самі файли, що в тесті 1 | ті самі 5 файлів | пройдено |
| 7 | `history` | рівно 10 результатів, оцінки не зростають | 10 результатів, від 8,685 (`List_of_historians_by_area_of_study`) до 7,705 | пройдено |
| 8 | `zymurgy` на веб-сторінці | у `expanded_terms` є нові слова, серед результатів є документ без `zymurgy` | `expanded_terms`: zymurgy, microbiology, fermentation, brewing, biotechnology. Першими йдуть `JS_Chiao`, `Homebrewing`, `Industrial_fermentation`, у яких немає `zymurgy` | пройдено |
| 9 | `zymurgy` на веб-сторінці при зупиненій Ollama | `expanded_terms` порожній, результати як у `./search` | `expanded_terms` порожній, ті самі 5 файлів, що в тесті 1 | пройдено |
| 10 | будь-який запит через `curl` | є `results[].doc_id`, `score`, `snippet` і `time_ms` | поля є в усіх відповідях тестів 1–7 | пройдено |

Усі шляхи мають вигляд `wikipedia/NNNN/<назва>.txt`.

## Як відтворити

Очікуване для тестів 1, 2 і 6:

```bash
docker compose run --rm dataset grep -rlwi zymurgy /data
```

Тести 1–7 і 10, із заміною тексту запиту:

```bash
curl -X POST http://localhost:8080/api/search -d '{"query": {"text": "zymurgy", "mode": "keyword"}}'
```

Тести 8 і 9 виконуються на http://localhost:8080. Для тесту 9 Ollama зупиняють командою `docker compose stop ollama`.
