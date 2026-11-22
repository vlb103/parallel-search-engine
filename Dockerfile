FROM debian:bookworm-slim AS build
RUN apt-get update && apt-get install -y --no-install-recommends \
      g++ cmake make libssl-dev nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY CMakeLists.txt .
COPY src src
COPY tests tests
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j

FROM build AS test
CMD ["sh", "-c", "setarch $(uname -m) -R ./build/index_race_test"]

FROM python:3.12-slim AS dataset
RUN pip install --no-cache-dir pyarrow==26.0.1
WORKDIR /app
COPY scripts/prepare_dataset.py scripts/check_dataset.py ./
CMD ["python", "prepare_dataset.py", "/data"]

FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y --no-install-recommends libssl3 \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=build /src/build/search_server .
COPY web web
EXPOSE 8080
CMD ["./search_server"]
