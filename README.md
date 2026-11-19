# Parallel Search Engine

Пошукова система, яка паралельно будує інвертований індекс з текстових файлів і відповідає кільком клієнтам одночасно. Сервер написано на C++17. Пул потоків і потокобезпечна хеш-таблиця для індексу реалізовані власноруч. Запит розширюється через локальну LLM (Ollama), а етапи пошуку передаються клієнту через WebSocket. Уся система запускається однією командою `docker compose up`.

## Можливості

- Паралельна побудова інвертованого індексу власним пулом потоків. Кількість потоків задається в запиті.
- Власна потокобезпечна хеш-таблиця для індексу: сегменти, у кожного свій `shared_mutex`.
- Пошук за ключовими словами з ранжуванням TF-IDF і фрагментом тексту для кожного результату.
- Одночасне обслуговування кількох клієнтів пулом потоків сервера.
- Розширення запиту ключовими словами та синонімами через Ollama (`/api/generate`).
- Стрімінг етапів пошуку через WebSocket.
- Два клієнти: консольний `./search "запит"` і веб-сторінка.

## Дані

Сервер індексує всі файли з каталогу `data/` рекурсивно, структура підкаталогів може бути будь-якою. Каталог не входить у репозиторій. За вимогами потрібно щонайменше 10 000 англійських текстових файлів загальним обсягом від 2 ГБ.

> **Тимчасово** використовується згенерований синтетичний датасет. Пізніше його буде замінено справжнім корпусом текстів.

Генерація (менше хвилини, 10 500 файлів, близько 2,06 ГБ, у `data/generated/`):

```bash
python scripts/generate_dataset.py
```

Кожен документ належить до однієї з десяти тем (space, medicine, finance, sport, music, cooking, climate, computing, history, travel) і складається з тематичних слів, загальновживаних англійських слів і псевдослів із розподілом за законом Ципфа. Генерація детермінована: з тим самим `--seed` виходять ті самі файли.

Перевірка обсягу:

```bash
python scripts/check_dataset.py data
```

Скрипт друкує кількість файлів і обсяг і завершується з кодом 1, якщо даних замало.

## Запуск

Потрібен Docker з Compose v2.

```bash
docker compose up --build
```

Піднімаються два сервіси: `server` (пошуковий сервер на порту 8080) і `ollama` (локальна LLM). Під час першого запуску Ollama завантажує модель `llama3.2:1b` (близько 1,3 ГБ) у том `ollama`, тому наступні запуски швидкі. Іншу модель можна задати змінною `OLLAMA_MODEL`:

```bash
OLLAMA_MODEL=llama3.2:3b docker compose up --build
```

Перевірка готовності:

```bash
curl -i http://localhost:8080/api/health
```

Зупинка: `Ctrl+C` або `docker compose down`.

## Використання

Приклади нижче виконано на тимчасовому датасеті. Відповіді справжні, у пошуку показано лише перші два результати з десяти.

### Перевірка стану

```bash
curl http://localhost:8080/api/health
```

```json
{"ollama":false,"status":"ok"}
```

### Побудова індексу

Після старту індекс порожній. Побудова у 8 потоків:

```bash
curl -X POST http://localhost:8080/api/index -d '{"threads": 8}'
```

```json
{"documents_indexed":10500,"status":"ok","time_ms":92469}
```

Запит синхронний: відповідь приходить, коли індекс побудовано. Поки триває побудова, повторний `POST /api/index` отримує `409`, а пошук уже працює за проіндексованими документами.

### Статистика

```bash
curl http://localhost:8080/api/stats
```

```json
{"documents":10500,"index_build_time_ms":92469,"threads_used":8}
```

### Пошук

```bash
curl -X POST http://localhost:8080/api/search -d '{"query": {"text": "telescope galaxy", "mode": "keyword"}}'
```

```json
{
  "results": [
    {
      "doc_id": "generated/space/doc_07406.txt",
      "score": 35.979843687958606,
      "snippet": " comet planet and.  And government star of and with to bior galaxy was in nosilu mars orbit.  On was antokive the on mean day on that for buarso andedunu doturima kava.  Galaxy for by and at people in"
    },
    {
      "doc_id": "generated/space/doc_07610.txt",
      "score": 35.95037486937771,
      "snippet": " bate child dain as in erkidere rocket kizidisa moon anmori telescope bukesosa.  Cosmos by day telescope star write to star andamo rizibive that family bazuta anenze.  Cosmos that enlebi time way thin"
    }
  ],
  "time_ms": 431
}
```

Той самий пошук у другому форматі запиту:

```bash
curl -X POST http://localhost:8080/api/search -d '{"query": "telescope galaxy", "mode": "keyword"}'
```

Помилки:

```bash
curl -X POST http://localhost:8080/api/index -d '{"threads": 0}'
```

```json
{"error":"threads must be an integer between 1 and 256"}
```

```bash
curl -X POST http://localhost:8080/api/search -d '{"query": "x", "mode": "semantic"}'
```

```json
{"error":"unsupported mode"}
```

### Пошук з консолі

Клієнт `search` потребує лише Python 3 без додаткових пакетів.

```bash
./search "telescope galaxy"
```

На Windows без WSL:

```bash
python search "telescope galaxy"
```

```
1. [35.980] generated/space/doc_07406.txt
    comet planet and.  And government star of and with to bior galaxy was in nosilu mars orbit.  On was antokive the on mean day on that for buarso andedunu doturima kava.  Galaxy for by and at people in
2. [35.950] generated/space/doc_07610.txt
    bate child dain as in erkidere rocket kizidisa moon anmori telescope bukesosa.  Cosmos by day telescope star write to star andamo rizibive that family bazuta anenze.  Cosmos that enlebi time way thin
...

10 results in 377 ms
```

Адреса сервера задається параметром `--url` або змінною `SEARCH_URL`:

```bash
./search "telescope" --url http://localhost:8080
```

## API

Усі відповіді в JSON.

### REST

| Ендпоінт | Тіло запиту | Відповідь 200 |
|---|---|---|
| `GET /api/health` | - | `{"status": "ok", "ollama": true}` |
| `POST /api/index` | `{"threads": 4}` | `{"status": "ok", "documents_indexed": 2000, "time_ms": 1234}` |
| `POST /api/search` | `{"query": {"text": "...", "mode": "keyword"}}` | `{"results": [{"doc_id": "...", "score": 0.95, "snippet": "..."}], "time_ms": 56}` |
| `GET /api/stats` | - | `{"documents": 2000, "index_build_time_ms": 1234, "threads_used": 4}` |

`POST /api/search` також приймає варіант `{"query": "...", "mode": "keyword"}`. Підтримується лише режим `keyword`.

Помилки мають вигляд `{"error": "..."}`:

| Код | Коли |
|---|---|
| 400 | Невалідний JSON, немає тексту запиту, `threads` поза межами 1–256, режим не `keyword` |
| 404 | Невідомий шлях або метод |
| 409 | Індексація вже триває |

### WebSocket `/ws/search`

Клієнт надсилає текстове повідомлення:

```json
{"query": "search text"}
```

Сервер відповідає трьома повідомленнями, по одному на кожен етап:

```json
{"stage": "query_expansion", "data": {"original": "...", "expanded_terms": ["..."]}}
{"stage": "keyword_search", "data": {"results": [{"doc_id": "...", "score": 1.2, "snippet": "..."}], "time_ms": 12}}
{"stage": "done"}
```

Елементи `results` мають той самий формат, що й у `POST /api/search`.
