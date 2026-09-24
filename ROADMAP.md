# Roadmap

This project is being developed step by step, starting with the core C++ backend and expanding into medical image handling, analysis and a web interface.

## Phase 1 - Core C++ Backend 🗹

- Set up the project structure
- Build the main C++ application
- Add user, patient and study classes
- Add file handling
- Use CMake for building the project

## Phase 2-10 - Testing 🗹

- Add unit and integration tests
- Test database operations
- Test DICOM metadata handling
- Improve error handling
- Run tests with CTest

## Phase 3 - Database 🗹

- Add SQLite support
- Store users, patients and studies
- Add CRUD operations
- Link patients to studies
- Link uploaded images to studies

## Phase 4 - Networking 🗹

- Build a TCP server
- Add REST API endpoints
- Support GET, POST, PUT and DELETE requests
- Handle binary image uploads
- Add request size limits

## Phase 5 - Medical Image Support 🗹

- Add DICOM support with DCMTK
- Import patient and study metadata
- Upload PNG, JPG, JPEG and DICOM files
- Store image information
- List images belonging to a study
- Re-open and delete stored images
- Read DICOM pixel data

## Phase 6 - Multithreading 🗹

- Handle multiple clients at the same time
- Use separate database connections for client threads
- Add SQLite busy handling
- Support concurrent requests

## Phase 7 - Security 🗹

- API key authentication
- API key stored in an environment variable
- Input and filename validation
- Request size validation
- Study and image relationship validation

## Phase 8 - CI and Docker 🗹

- Set up GitHub Actions
- Build the project automatically
- Run tests automatically
- Add Docker support

## Phase 9 - Web Interface 🗹

- Create a React and Vite frontend
- Add a dashboard
- Manage patients, studies and users
- Import DICOM metadata
- Upload and browse study images
- Analyse and delete uploaded images
- Show backend connection status

## Phase 10 - Image Analysis 🗹

- Add Python-based image processing
- Load DICOM, PNG and JPEG images
- Convert images to grayscale
- Add K-Means segmentation
- Display original and segmented images
- Show cluster centers and pixel counts
- Add a built-in CT demo
