/*
Manual REST API tests with curl

Start server first:

    ./build/med

PATIENTS

    curl -X POST http://localhost:1337/patients -d "2 Johan 30"
    curl http://localhost:1337/patients
    curl -X PUT http://localhost:1337/patients -d "2 Johan_Updated 31"
    curl http://localhost:1337/patients
    curl -X DELETE http://localhost:1337/patients -d "2"


USERS

    curl -X POST http://localhost:1337/users -d "1 kevin admin"
    curl http://localhost:1337/users
    curl -X PUT http://localhost:1337/users -d "1 kevin doctor"
    curl http://localhost:1337/users
    curl -X DELETE http://localhost:1337/users -d "1"


STUDIES
Patient 1 must exist first (foreign key)

    curl -X POST http://localhost:1337/studies -d "10 1 Brain_MRI"
    curl http://localhost:1337/studies
    curl -X PUT http://localhost:1337/studies -d "10 1 Updated_Brain_MRI"
    curl http://localhost:1337/studies
    curl -X DELETE http://localhost:1337/studies -d "10"

EXTRA

    Add -i flag to include all HTTP response headers
*/