# Parallel Search Engine

Пошукова система на C++17. Вона паралельно будує інвертований індекс з текстових файлів і обслуговує кількох клієнтів одночасно. Пул потоків і потокобезпечну хеш-таблицю для індексу написано власноруч. Запит розширюється через локальну LLM (Ollama), а етапи пошуку приходять клієнту через WebSocket.

## Запуск

```bash
docker compose up --build
```

Перший запуск завантажує датасет (1,6 ГБ) і модель `llama3.2:1b` (1,3 ГБ), далі вони беруться з томів. Сервер слухає порт 8080.

Після старту індекс порожній. Побудова у 8 потоків:

```bash
curl -X POST http://localhost:8080/api/index -d '{"threads": 8}'
```

## Пошук

Веб-сторінка відкривається на http://localhost:8080.

З консолі потрібен лише Python 3. На Windows без WSL команда починається з `python search`.

```bash
./search "moon landing"
```

Через REST:

```bash
curl -X POST http://localhost:8080/api/search -d '{"query": {"text": "moon landing", "mode": "keyword"}}'
```

## Дані

Англійська Wikipedia, дамп 20231101 з [wikimedia/wikipedia](https://huggingface.co/datasets/wikimedia/wikipedia), шарди 00016, 00037 і 00039, ліцензія CC BY-SA 3.0. Одна стаття в одному файлі: 468 864 файли, 2,63 ГБ. Їх готує сервіс `dataset` у томі `dataset`.

Перевірка обсягу:

```bash
docker compose run --rm dataset python check_dataset.py /data
```

## API

| Ендпоінт | Тіло запиту | Відповідь 200 |
|---|---|---|
| `GET /api/health` | - | `{"status": "ok", "ollama": true}` |
| `POST /api/index` | `{"threads": 4}` | `{"status": "ok", "documents_indexed": 2000, "time_ms": 1234}` |
| `POST /api/search` | `{"query": {"text": "...", "mode": "keyword"}}` | `{"results": [{"doc_id": "...", "score": 0.95, "snippet": "..."}], "time_ms": 56}` |
| `GET /api/stats` | - | `{"documents": 2000, "index_build_time_ms": 1234, "threads_used": 4}` |

`POST /api/search` приймає і `{"query": "...", "mode": "keyword"}`. Інших режимів, крім `keyword`, немає.

Помилки мають вигляд `{"error": "..."}`: 400 для некоректного запиту, 404 для невідомого шляху, 409 якщо індексація вже триває.

WebSocket `/ws/search` приймає `{"query": "..."}` і відповідає трьома повідомленнями:

```json
{"stage": "query_expansion", "data": {"original": "...", "expanded_terms": ["..."]}}
{"stage": "keyword_search", "data": {"results": [...], "time_ms": 12}}
{"stage": "done"}
```
