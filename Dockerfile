FROM ubuntu:24.04

WORKDIR /app

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libsqlite3-dev \
    libdcmtk-dev \
    python3 \
    python3-pip \
    python3-venv \
    && rm -rf /var/lib/apt/lists/*

COPY . .

RUN python3 -m venv .venv

RUN .venv/bin/pip install \
    pydicom \
    numpy \
    matplotlib \
    scikit-learn

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

RUN cmake --build build

ENV MPLBACKEND=Agg

EXPOSE 1337

CMD ["./build/med"]