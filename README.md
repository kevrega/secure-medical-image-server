# Secure Medical Image Server

A project for storing and managing medical images together with patient and study information.

The backend is written in C++ and uses SQLite for storage. It also supports DICOM files, a REST API, multithreading, automated tests, Docker and GitHub Actions.

## Features

Currently implemented:

* Patient, user and study management
* SQLite database
* TCP/HTTP server
* REST API
* Multithreaded client handling
* API key authentication
* DICOM metadata reading with DCMTK
* Importing patient and study information from DICOM files
* Python-based image analysis
* Unit tests with GoogleTest
* Docker support
* CI with GitHub Actions

## Technologies

* C++17
* CMake
* SQLite
* DCMTK
* GoogleTest
* Python
* pydicom
* Docker
* GitHub Actions
* Linux

## Build

Configure the project:

```bash
cmake -S . -B build
```

Build it:

```bash
cmake --build build
```

Set an API key:

```bash
export MEDICAL_API_KEY=test123
```

Start the server:

```bash
./build/med
```

The server runs on port `1337`.

## Docker

Build the image:

```bash
docker build -t medical-server .
```

Run it:

```bash
docker run --rm \
-p 1337:1337 \
-e MEDICAL_API_KEY=test123 \
medical-server
```

The server can then be reached at:

```text
http://localhost:1337
```

For example:

```bash
curl http://localhost:1337/patients \
-H "X-API-Key: test123"
```

## DICOM

DICOM files in the `data` directory can be imported through the API.

Example:

```bash
curl -X POST http://localhost:1337/dicom \
-H "X-API-Key: test123" \
--data "test.dcm"
```

The DICOM metadata is read and the patient and study information is added to the database.

## Tests

Run the tests with:

```bash
ctest --test-dir build --output-on-failure
```

Tests currently cover patients, users, studies, file handling, database operations and DICOM metadata reading.

## CI

GitHub Actions builds the project and runs the tests automatically when changes are pushed to the repository or when a pull request is created.

